import math
import time
import array
import board
import audioio
import analogio
import displayio
import digitalio
import terminalio
import gamepadshift
import neopixel_write
from adafruit_display_text import label
from adafruit_bitmap_font  import bitmap_font

# setup GPIO and font

noto18 = bitmap_font.load_font('/fonts/Noto-18.bdf')
noto18.load_glyphs('1234567890: '.encode('utf-8'))

neopixel           = digitalio.DigitalInOut(board.NEOPIXEL)
neopixel.direction = digitalio.Direction.OUTPUT
NEOPIXEL_LEN       = 5

light_in = analogio.AnalogIn(board.LIGHT)
audio    = audioio.AudioOut(board.A0)

pad = gamepadshift.GamePadShift(digitalio.DigitalInOut(board.BUTTON_CLOCK),
                                digitalio.DigitalInOut(board.BUTTON_OUT),
                                digitalio.DigitalInOut(board.BUTTON_LATCH))

speaker_enable           = digitalio.DigitalInOut(board.SPEAKER_ENABLE)
speaker_enable.direction = digitalio.Direction.OUTPUT
speaker_enable.value     = True

# button constants

BUTTON_LEFT  = const(128)
BUTTON_UP    = const(64)
BUTTON_DOWN  = const(32)
BUTTON_RIGHT = const(16)
BUTTON_SEL   = const(8)
BUTTON_START = const(4)
BUTTON_A     = const(2)
BUTTON_B     = const(1)

# state constants

CONFIG         = const(1)
COUNTDOWN      = const(2)
END            = const(3) # currently not used

# colour constants

C_DEFAULT  = const(0x2222ff)
C_DISABLED = const(0x555555)
C_SELECTED = const(0xff5500)

# timers and retimer constants

RETIMER     = const(0)
TIMER1      = const(1)
TIMER2      = const(2)
TIMERS_LEN  = const(3)
NOT_RETIMER = const(3)
MIN_TIMER   = const(0)
MAX_TIMER   = const(24 * 60 * 60)
IDL_TIMER   = const(-1)

# utilities

def light_moving_average(size):
  global light_in
  queue = []
  while True:
    value = light_in.value
    queue.insert(0, value)
    if len(queue) > size:
      queue.pop()
    yield (value, sum(queue) / len(queue))


def hor_min_sec(seconds):
  min = math.floor(seconds / 60)
  return (math.floor(min / 60), min % 60, seconds % 60)


def labels(active = [True, True, True]):
  result = displayio.Group(max_size=TIMERS_LEN)
  for i, text in enumerate(('retimer', 'timer 1', 'timer 2')):
    text_area = label.Label(terminalio.FONT, text=text, color=C_DEFAULT, x=110, y=(i + 1) * 35)
    text_area.color = C_DEFAULT if active[i] else C_DISABLED
    result.append(text_area)
  return result

# configuration screen

def config_screen(timers, selection):
  def row(seconds, y, local_selection):
    global noto18
    hor, min, sec = hor_min_sec(seconds)
    texts = (
      ('{:02d}'.format(hor), 1, 1),
      (':', None, 3),
      ('{:02d}'.format(min), 2, 1),
      (':', None, 3),
      ('{:02d}'.format(sec), 3, 1)
    )
    x = 10
    result = displayio.Group(max_size=5)
    for txt, pos, kern in texts:
      text_label = label.Label(noto18, text=txt ,x=x, y=y)
      text_label.color = C_SELECTED if local_selection == pos else C_DEFAULT
      result.append(text_label)
      bx, by, w, h = text_label.bounding_box
      x += w + kern
    return result

  result = displayio.Group(max_size=TIMERS_LEN + 1)
  result.append(labels())
  for i, timer in enumerate(timers):
    result.append(row(timer, (i + 1) * 35, selection - (i * 3)))
  return result


def config_loop(timers = [0, 0, 0], selection = 3):
  global pad

  def inc_by(amount):
    if selection in (3, 6, 9): # seconds
      return amount
    elif selection in (2, 5, 8): # minutes
      return amount * 60
    elif selection in (1, 4, 7): # hours
      return 60 * 60
    else:
      assert False

  # initial config screen
  board.DISPLAY.show(config_screen(timers, selection))
  prev_buttons = 0
  repeat_count = 0
  repeat_delay = 0
  while True:
    buttons = pad.get_pressed()
    increment = 0
    # interpret button press, with logic to increment faster if a button is held down
    if repeat_count == 0:
      if buttons != prev_buttons:
        increment = inc_by(1)
        repeat_count = 1
        repeat_delay = time.monotonic() + 0.5
    elif time.monotonic() > repeat_delay and buttons == prev_buttons:
      if repeat_count == 1:
        increment = inc_by(4)
      elif repeat_count in (2, 3):
        increment = inc_by(5)
      elif repeat_count > 3:
        increment = inc_by(10)
      repeat_count = max(4, repeat_count + 1)
      repeat_delay = time.monotonic() + 0.5
    elif buttons != prev_buttons:
      repeat_count = 0
      repeat_delay = 0
    prev_buttons = buttons
    # update the values and repaint the screen, if needed
    if increment != 0:
      if buttons & BUTTON_RIGHT > 0:
        selection = min(9, selection + 1)
      elif buttons & BUTTON_LEFT > 0:
        selection = max(1, selection - 1)
      elif buttons & BUTTON_UP > 0 or buttons & BUTTON_DOWN > 0:
        if buttons & BUTTON_DOWN > 0:
          increment = -increment
        selection_timer = math.floor((selection - 1) / 3)
        timers[selection_timer] = min(MAX_TIMER, max(MIN_TIMER, timers[selection_timer] + increment))
      if buttons & BUTTON_START > 0:
        return (COUNTDOWN, timers)
      else:
        board.DISPLAY.show(config_screen(timers, selection))

# countdown (and alarm) screen

Blank_np   = bytearray([0, 0, 0] * NEOPIXEL_LEN)
Retimer_np = bytearray([200, 0, 100] * NEOPIXEL_LEN)
Timer_np   = bytearray([150, 150, 0] * NEOPIXEL_LEN)


def alarm_np(target):
  while True:
    yield Retimer_np if target == RETIMER else Timer_np
    yield Blank_np
    yield None


def alarm_audio():
  tone_volume = 1  # Increase this to increase the volume of the tone.
  frequency = 440  # Set this to the Hz of the tone you want to generate.
  length = 8000 // frequency
  sine_wave = array.array("H", [0] * length)
  for i in range(length):
    sine_wave[i] = int((1 + math.sin(math.pi * 2 * i / length)) * tone_volume * (2 ** 15 - 1))
  return audioio.RawSample(sine_wave)


def clear_alarm():
  global audio, neopixel
  audio.stop()
  neopixel_write.neopixel_write(neopixel, Blank_np)


def countdown_screen(timers, tick):
  DEFAULT  = const(0)
  SELECTED = const(1)
  DISABLED = const(2)
  Colours = [C_DEFAULT, C_SELECTED, C_DISABLED]

  def row(seconds, y, flag):
    global noto18
    hor, min, sec = hor_min_sec(seconds)
    if tick:
      template = '{:02d}:{:02d}:{:02d}'
    else:
      template = '{:02d} {:02d} {:02d}'
    text_area = label.Label(noto18, max_glyphs=8, x=10, y=y)
    text_area.color = Colours[state]
    text_area.text = template.format(hor, min, sec)
    return text_area

  result = displayio.Group(max_size=TIMERS_LEN + 1)
  active = []
  for i, timer in enumerate(timers):
    active.append(timer >= 0)
    if timer == 0:
      state = SELECTED
    elif timer > 0:
      state = DEFAULT
    else:
      state = DISABLED
    result.append(row(max(0, timer), (i + 1) * 35, state))
  result.append(labels(active))
  return result


def countdown_loop(timers):
  global pad, audio, neopixel

  def idle_or_current(start_time):
    from_start_time = time.time() - start_time
    current = lambda t: max(MIN_TIMER, t - from_start_time)
    return (current, lambda t: IDL_TIMER if t == 0 else current(t))

  # since there are 2 timers and the retimer, when one of them stops, we need specify
  # logic to compute how much time is left on the others
  def re_timers(retimers, timers, curr):
    result = []
    for i, t in enumerate(retimers):
      if i == RETIMER and curr(retimers[RETIMER]) == 0:
          result.append(timers[i])
      else:
        result.append(curr(t))
    return (time.time(), result)

  alarm = None
  prev_buttons = 0
  retimers = timers
  configs = list(timers)
  start_time = time.time() # when the countdown starts
  tick_delay = float('-inf')
  light_average = light_moving_average(5)
  while True:
    stop_retimer = stop_timer = False
    current, ioc = idle_or_current(start_time)
    light, avg = next(light_average)
    # interpret button press or light-based gesture
    buttons = pad.get_pressed()
    if buttons != prev_buttons:
      prev_buttons = buttons
      if buttons & BUTTON_SEL > 0:
        clear_alarm()
        return (CONFIG, list(map(current, retimers)))
      else:
        stop_timer = buttons & BUTTON_A > 0
        stop_retimer  = buttons & BUTTON_B > 0
    stop_timer = stop_timer or light < avg * 0.66
    if alarm == None:
      # alarm currently off, should we start it?
      if any(ioc(t) == MIN_TIMER for t in retimers):
        audio.play(alarm_audio(), loop=True)
        if ioc(retimers[RETIMER]) == MIN_TIMER:
          alarm = alarm_np(RETIMER)
        else:
          alarm = alarm_np(NOT_RETIMER)
    else:
      # alarm ongoing, stop it if...
      if stop_timer or stop_retimer:
        alarm = None
        clear_alarm()
        # ...the stop button is pressed for the retimer...
        if stop_retimer and ioc(retimers[RETIMER]) == MIN_TIMER:
          # ...timer1 or timer2 still ongoing, stops only the retimer...
          if any(ioc(t) > 0 for t in retimers[1:]):
            timers[RETIMER] = 0
            start_time, retimers = re_timers(retimers, timers, current)
            curr, ioc = idle_or_current(start_time)
          # ...timer1 and timer2 also stopped, returns to config...
          else:
            return (CONFIG, configs)
        # ...else it's for timer1, timer2 or it's the continue button for retimer...
        else:
          # ...we restart a countdown (timer1 or timer2 still counting or retimer restarts)...
          if ioc(retimers[RETIMER]) >= 0 or any(ioc(t) > 0 for t in retimers[1:]):
            start_time, retimers = re_timers(retimers, timers, current)
            curr, ioc = idle_or_current(start_time)
          # ...but we stop if all timers are over
          else:
            return (CONFIG, configs)
      else:
      # alarm ongoing, not stopped so animate the neopixels
        colour = next(alarm)
        if colour != None:
          neopixel_write.neopixel_write(neopixel, colour)
    # repaint the screen (roughly) every second so the ':' ticks in and off
    if time.time() > tick_delay:
      tick_delay = time.time()
      board.DISPLAY.show(countdown_screen(map(ioc, retimers), time.time() % 2))      
    if alarm == None:
      time.sleep(1)
    else:
      # shorter sleep to support alarm animation
      time.sleep(0.3)

# main

state = CONFIG
timers = [0, 0, 0]

while state != END:
  if state == CONFIG:
    state, timers = config_loop(timers)
  elif state == COUNTDOWN:
    state, timers = countdown_loop(timers)
