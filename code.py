import math
import time
import array
import board
import audioio
import digitalio
import neopixel_write
from adafruit_display_text import label
from adafruit_bitmap_font import bitmap_font

font = bitmap_font.load_font('/fonts/Noto-18.bdf')
font.load_glyphs('1234567890: '.encode('utf-8'))

np_pin = digitalio.DigitalInOut(board.NEOPIXEL)
np_pin.direction = digitalio.Direction.OUTPUT
end_time = time.time() + int(60 * 0.25)
display = board.DISPLAY

tone_volume = 1  # Increase this to increase the volume of the tone.
frequency = 440  # Set this to the Hz of the tone you want to generate.
length = 8000 // frequency
sine_wave = array.array("H", [0] * length)
for i in range(length):
    sine_wave[i] = int((1 + math.sin(math.pi * 2 * i / length)) * tone_volume * (2 ** 15 - 1))
audio = audioio.AudioOut(board.A0)
sine_wave_sample = audioio.RawSample(sine_wave)

speaker_enable = digitalio.DigitalInOut(board.SPEAKER_ENABLE)
speaker_enable.direction = digitalio.Direction.OUTPUT
speaker_enable.value = True

def colour(p, minutes):
  if p < minutes + 1:
    if minutes == 0:
      return (0, 100, 0)
    else:
      return (100, 0, 0)
  else:
    return (0, 0, 0)

diff = float('inf')
previous_minutes = float('inf')

while diff > 0:
  diff = end_time - time.time()
  minutes = math.floor(diff / 60)
  seconds = diff % 60
  if diff % 2:
    text = '{}:{:02d}'.format(minutes, seconds)
  else:
    text = '{} {:02d}'.format(minutes, seconds)
  text_area = label.Label(font, text=text, color=0x0000ff)
  text_area.x = 10
  text_area.y = 80
  display.show(text_area)
  if minutes != previous_minutes:
    neopixel_write.neopixel_write(np_pin, bytearray([i for p in range(0, 5) for i in colour(p, minutes)]))
  previous_minutes = minutes
  time.sleep(1)
  
audio.play(sine_wave_sample, loop=True)
for i in range(10):
  if i % 2:
    colour = (0, 255, 0)
  else:
    colour = (0, 0, 0)
  neopixel_write.neopixel_write(np_pin, bytearray([i for p in range(0, 5) for i in colour]))
  time.sleep(1)
audio.stop()
