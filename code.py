import math
import time
import array
import board
import audioio
import analogio
import displayio
import digitalio
import gamepadshift
import neopixel_write
from adafruit_display_text import label
from adafruit_bitmap_font  import bitmap_font

# setup

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

prev_buttons = pad.get_pressed()
press_delay  = 0

# button constants

BUTTON_LEFT  = const(128)
BUTTON_UP    = const(64)
BUTTON_DOWN  = const(32)
BUTTON_RIGHT = const(16)
BUTTON_SEL   = const(8)
BUTTON_START = const(4)
BUTTON_A     = const(2)
BUTTON_B     = const(1)

# states

CONFIG         = const(1)
COUNTDOWN      = const(2)
ALARM          = const(3)
ALARM_LOOP     = const(4)
END            = const(5)

# timers and retimer

timers = [0, 0, 0]

RETIMER   = const(0)
TIMER1    = const(1)
TIMER2    = const(2)
A_TIMER   = const(3)
MIN_TIMER = const(0)
MAX_TIMER = const(24 * 60 * 60)
IDL_TIMER = const(-1)

# utilities

def light_moving_average(size):
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

# configuration screen

def config_screen(timers, selection):
  def row(seconds, y, local_selection):
    hor, min, sec = hor_min_sec(seconds)

    labels = (
      ('{:02d}'.format(hor), 1, 1),
      (':', None, 3),
      ('{:02d}'.format(min), 2, 1),
      (':', None, 3),
      ('{:02d}'.format(sec), 3, 1)
    )

    x = 10
    result = displayio.Group(max_size=5)
    for st, pos, kern in labels:
      text_label = label.Label(noto18, text=st ,x=x, y=y)
      text_label.color = 0xff5500 if local_selection == pos else 0x0000ff
      result.append(text_label)
      bx, by, w, h = text_label.bounding_box
      x += w + kern

    return result

  result = displayio.Group()
  for i, timer in enumerate(timers):
    result.append(row(timer, (i + 1) * 30, selection - (i * 3)))
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

  # initially request a display
  board.DISPLAY.show(config_screen(timers, selection))

  prev_buttons = 0
  repeat_count = 0
  repeat_delay = 0

  while True:
    buttons = pad.get_pressed()

    increment = 0
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

Blank_np = bytearray([0, 0, 0] * NEOPIXEL_LEN)
Retimer_np = bytearray([200, 0, 100] * NEOPIXEL_LEN)
Timer_np = bytearray([150, 150, 0] * NEOPIXEL_LEN)

def alarm_np(target):
  while True:
    for count in range(3):
      if count == 0:
        yield Retimer_np if target == RETIMER else Timer_np
      else:
        yield Blank_np


def alarm_audio():
  global audio

  tone_volume = 1  # Increase this to increase the volume of the tone.
  frequency = 440  # Set this to the Hz of the tone you want to generate.
  length = 8000 // frequency
  sine_wave = array.array("H", [0] * length)
  for i in range(length):
    sine_wave[i] = int((1 + math.sin(math.pi * 2 * i / length)) * tone_volume * (2 ** 15 - 1))
  return audioio.RawSample(sine_wave)


def countdown_screen(timers, tick):
  def row(seconds, y, active):
    hor, min, sec = hor_min_sec(seconds)
    if tick:
      template = '{:02d}:{:02d}:{:02d}'
    else:
      template = '{:02d} {:02d} {:02d}'

    text_area = label.Label(noto18, max_glyphs=8, x=10, y=y)
    text_area.color = 0x0000ff if active else 0x555555
    text_area.text = template.format(hor, min, sec)
    return text_area

  timers = list(timers)
  print('repaint {}'.format(','.join(str(t) for t in timers)))
  result = displayio.Group()
  for i, timer in enumerate(timers):
    result.append(row(max(0, timer), (i + 1) * 30, timer >= 0))
  return result


def re_timers(retimers, timers, curr):
  result = []
  for i, t in enumerate(retimers):
    if i == RETIMER:
      print(t)
      result.append(timers[i])
    else:
      result.append(curr(t))
  return result


def countdown_loop(timers):
  global pad

  start_time = time.time()
  retimers = timers

  def current():
    from_start_time = time.time() - start_time
    return lambda t: max(MIN_TIMER, t - from_start_time)

  def idle_or_current():
    curr = current()
    return lambda t: IDL_TIMER if t == 0 else curr(t)

  alarm = None
  prev_buttons = 0
  tick_delay = float('-inf')
  light_average = light_moving_average(5)

  while True:
    restart_btn = False
    value, avg = next(light_average)

    buttons = pad.get_pressed()
    if buttons != prev_buttons:
      prev_buttons = buttons
      if buttons & BUTTON_SEL > 0:
        return (CONFIG, list(map(current(), retimers)))
      else:
        restart_btn = buttons & BUTTON_A > 0 or buttons & BUTTON_B > 0

    ioc = idle_or_current()
    if any(ioc(t) == MIN_TIMER for t in retimers):
      board.DISPLAY.show(countdown_screen(map(ioc, retimers), True))
      audio.play(alarm_audio(), loop=True)
      if alarm == None:
        if ioc(retimers[RETIMER]) == MIN_TIMER:
          alarm = alarm_np(RETIMER)
        else:
          alarm = alarm_np(A_TIMER)
      retimers = re_timers(retimers, timers, current())
      start_time = time.time()
      print(retimers)

    if alarm != None:
      if restart_btn or value < avg * 0.66:
        alarm = None
        audio.stop()
        neopixel_write.neopixel_write(neopixel, Blank_np)
        if all(t == 0 for t in retimers[1:]):
          return (CONFIG, timers)
      else:
        neopixel_write.neopixel_write(neopixel, next(alarm))
    elif time.time() > tick_delay:
      tick_delay = time.time()
      board.DISPLAY.show(countdown_screen(map(ioc, retimers), time.time() % 2))
      
    if alarm == None:
      time.sleep(1)
    else:
      time.sleep(0.3)

# main

state = CONFIG

while state != END:
  if state == CONFIG:
    state, timers = config_loop(timers)
  elif state == COUNTDOWN:
    state, timers = countdown_loop(timers)
