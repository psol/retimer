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
until_timers = [0, 0, 0]
diff_timers  = [0, 0, 0]

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


def time_left(t):
  return 0 if t == 0 else max(0, t - time.time())

# configuration screen

prev_buttons = pad.get_pressed()
prev_press   = 0
press_delay  = 0
selection    = 3


def display_group_config():
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

  global timers, selection

  result = displayio.Group(max_size=len(timers))
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

def display_group_countdown(tick):
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

  result = displayio.Group(max_size=len(diff_timers))
  for i, timer in enumerate(diff_timers):
    result.append(row(timer, (i + 1) * 30, timers[i] != 0))
  return result

# alarm

def alarm():
  tone_volume = 1  # Increase this to increase the volume of the tone.
  frequency = 440  # Set this to the Hz of the tone you want to generate.
  length = 8000 // frequency
  sine_wave = array.array("H", [0] * length)
  for i in range(length):
    sine_wave[i] = int((1 + math.sin(math.pi * 2 * i / length)) * tone_volume * (2 ** 15 - 1))
  audio = audioio.AudioOut(board.A0)
  sine_wave_sample = audioio.RawSample(sine_wave)

  light_average = moving_average(5)
  next(light_average)  # prime coroutine

  audio.play(sine_wave_sample, loop=True)
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
  audio.stop()

# main

board.DISPLAY.show(display_group_config())

action = read_config_buttons() 
while action >= 0:
  if action > 0:
    board.DISPLAY.show(display_group_config())
  action = read_config_buttons()

until_timers = list(map(lambda t: 0 if t == 0 else time.time() + t, timers))

diff_timers = list(map(time_left, until_timers))
while any(t > 0 for t in diff_timers):
  time.sleep(1)
  diff_timers = list(map(time_left, until_timers))
  board.DISPLAY.show(display_group_countdown(time.time() % 2))

alarm()
