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
from adafruit_bitmap_font import bitmap_font

# setup

noto18 = bitmap_font.load_font('/fonts/Noto-18.bdf')
noto18.load_glyphs('1234567890: '.encode('utf-8'))

neopixel = digitalio.DigitalInOut(board.NEOPIXEL)
neopixel.direction = digitalio.Direction.OUTPUT

light_in = analogio.AnalogIn(board.LIGHT)

pad = gamepadshift.GamePadShift(digitalio.DigitalInOut(board.BUTTON_CLOCK),
                                digitalio.DigitalInOut(board.BUTTON_OUT),
                                digitalio.DigitalInOut(board.BUTTON_LATCH))

speaker_enable = digitalio.DigitalInOut(board.SPEAKER_ENABLE)
speaker_enable.direction = digitalio.Direction.OUTPUT
speaker_enable.value = True

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

# timers and retimer

timers       = [0, 0, 0]

RETIMER   = 0
TIMER1    = 1
TIMER2    = 2
MAX_TIMER = 24 * 60 * 60

# utilities

def moving_average(size):
  queue = []
  avg = None

  while True:
    value = yield avg
    queue.append(value)
    if len(queue) > size:
      queue.pop()
    avg = sum(queue) / len(queue)


def hor_min_sec(seconds):
  min = math.floor(seconds / 60)
  return (math.floor(min / 60), min % 60, seconds % 60)


def remain_time(t):
  return -1 if t == 0 else max(0, t - time.time())

# configuration screen

prev_press   = 0
selection    = 3


def display_group_config(timers, selection):
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


def read_config_buttons():
  global pad, prev_buttons, press_delay, prev_press, timers, selection

  def inc_by(amount):
    if selection in (3, 6, 9): # seconds
      return amount
    elif selection in (2, 5, 8): # minutes
      return amount * 60
    elif selection in (1, 4, 7): # hours
      return 60 * 60
    else:
      assert False

  buttons = pad.get_pressed()

  increment = 0
  if prev_press == 0:
    if buttons != prev_buttons:
      increment = inc_by(1)
      prev_press = 1
  elif time.monotonic() > press_delay and buttons == prev_buttons:
    if prev_press == 1:
      increment = inc_by(4)
    elif prev_press in (2, 3):
      increment = inc_by(5)
    elif prev_press > 3:
      increment = inc_by(10)
    if prev_press < 4:
      prev_press += 1
  elif buttons != prev_buttons:
    prev_press = 0
  prev_buttons = buttons

  if increment != 0:
    press_delay = time.monotonic() + 0.5
    if (buttons & BUTTON_RIGHT) > 0:
      selection = min(9, selection + 1)
    elif (buttons & BUTTON_LEFT) > 0:
      selection = max(1, selection - 1)
    elif (buttons & BUTTON_UP) > 0 or (buttons & BUTTON_DOWN) > 0:
      if (buttons & BUTTON_DOWN) > 0:
        increment = -increment
      selected_timer = math.floor((selection - 1) / 3)
      timers[selected_timer] = min(MAX_TIMER, max(0, timers[selected_timer] + increment))
    elif (buttons & BUTTON_START) > 0:
      return -1
    return 1
  else:
    return 0

# countdown screen

tick_delay = 0

def display_group_countdown(timers, tick):
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

  result = displayio.Group()
  for i, timer in enumerate(timers):
    result.append(row(max(0, timer), (i + 1) * 30, timer >= 0))
  return result


def read_countdown_buttons():
  global pad, prev_buttons, tick_delay, press_delay

  result = 0

  buttons = pad.get_pressed()
  if buttons != prev_buttons:
    if (buttons & BUTTON_SEL) > 0:
      result = -1
    press_delay = time.monotonic() + 0.5
  prev_buttons = buttons

  if time.time() > tick_delay:
    result = 1
    tick_delay = time.time()
  
  return result

# alarm

light_average = moving_average(5)
next(light_average)  # prime coroutine
compare_avg = -1
stop_at = 0
compare_at = float('inf')
audio = audioio.AudioOut(board.A0)

def start_alarm():
  global audio, stop_at

  tone_volume = 1  # Increase this to increase the volume of the tone.
  frequency = 440  # Set this to the Hz of the tone you want to generate.
  length = 8000 // frequency
  sine_wave = array.array("H", [0] * length)
  for i in range(length):
    sine_wave[i] = int((1 + math.sin(math.pi * 2 * i / length)) * tone_volume * (2 ** 15 - 1))
  sine_wave_sample = audioio.RawSample(sine_wave)
  stop_at = time.monotonic() + 300
  audio.play(sine_wave_sample, loop=True)


def read_alarm_light():
  global compare_avg, compare_at, light_average, audio, stop_at
  
  if time.monotonic() > stop_at:
    neopixel_write.neopixel_write(neopixel, bytearray([i for p in range(0, 5) for i in (0, 0, 0)]))
    return -1
  elif time.monotonic() > compare_at:
    value = light_in.value
    if value < compare_avg:
      neopixel_write.neopixel_write(neopixel, bytearray([i for p in range(0, 5) for i in (255, 0, 0)]))
      time.sleep(0.3)
      neopixel_write.neopixel_write(neopixel, bytearray([i for p in range(0, 5) for i in (0, 0, 0)]))
      return -1
    return 0
  elif compare_avg < 0:
    value = light_in.value
    avg = light_average.send(value)
    if value < avg * 0.66:
      audio.stop()
      neopixel_write.neopixel_write(neopixel, bytearray([i for p in range(0, 5) for i in (0, 0, 255)]))
      compare_avg = avg * 0.66
      compare_at = time.monotonic() + 0.3
    return 0
  elif (pad.get_pressed() & BUTTON_START) > 0:
    neopixel_write.neopixel_write(neopixel, bytearray([i for p in range(0, 5) for i in (0, 0, 0)]))
    audio.stop()
    return -1
  else:
    return 0


def play_alarm():
  sec = prev_sec = time.time()
  for i in range(100):
    value = light_in.value
    avg = light_average.send(value)
    if value < (avg * 0.66):
      audio.stop()
      neopixel_write.neopixel_write(neopixel, bytearray([i for p in range(0, 5) for i in (0, 0, 255)]))
      time.sleep(2)
      break
    sec = time.time()
    if sec != prev_sec:
      colour = (0, 255, 0)
    else:
      colour = (0, 0, 0)
    prev_sec = sec
    neopixel_write.neopixel_write(neopixel, bytearray([i for p in range(0, 5) for i in colour]))
    time.sleep(0.1)

# main

board.DISPLAY.show(display_group_config(timers, selection))
action = read_config_buttons() 
state = 0
until_timers = [0] * len(timers)

while state >= -1:
  if state == 0:
    if action > 0:
      board.DISPLAY.show(display_group_config(timers, selection))
    if action < 0:
      state = 1
      until_timers = list(map(lambda t: 0 if t == 0 else time.time() + t, timers))
      action = read_countdown_buttons()
    else:
      action = read_config_buttons()
  elif state == 1:
    if action > 0:
      now = time.time()
      if all(now > t for t in until_timers):
        start_alarm()
        state = -1
        action = read_alarm_light()
      board.DISPLAY.show(display_group_countdown(map(remain_time, until_timers), time.time() % 2))
      continue
    if action < 0:
      state = 0
      action = read_config_buttons()
      board.DISPLAY.show(display_group_config(timers, selection))
    else:
      action = read_countdown_buttons()
  elif state == -1:
    if action == 0:
      action = read_alarm_light()
    elif action < 0:
      state = -2
