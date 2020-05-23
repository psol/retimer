# retimer
When I cook and particularly when I grill vegetables, I need a regular reminder to check
on the food. Enter the retimer. The retimer is a cooking timer that re-triggers every
x minutes to check on the grilling and, usually, toss the vegetables around.

Same thing with no-knead bread who needs a fold every 20-25 minutes.

For convenient, two regular timers are also supported, typically timer 1 goes off when
cooking should be finished and timer 2 a little bit earlier to add aromatics.
For bread-making, timer 1 goes off when the dough should be proofed while timer 2
is shaping time.

Last but not least, I wanted hands-free control of the retimer. Obstructing the light
sensor is equivalent to pressing button A (restart).

## UX Prototype
A kitchen timer is not a particulary difficult project. In fact I have had a similar
workflow for some time: I set my kitchen timer to 5 minutes and when it goes off,
I verify my grilling and restart the timer.

Expect I sometimes forget to restart the timer.

OKay, I often forget to restart the timer.

So the retimer aims to achieve a better UX for the task of grilling vegetables.
When designing UX, it's important to formulate hypothesis and validate them through
prototypes. Paper prototypes would not real work here so I had to build something fast.
Which leads us to hardware.

## Hardware
retimer is developed on the [Adafruit PyBadge](https://www.adafruit.com/product/4200),
in [CircuitPython 5.3.0](https://circuitpython.org).

The PyBadge has plenty of buttons, a light sensor (hands-free operation), a screen,
neopixels, on-board audio and sophisticated power management that includes a battery
connector. It is also robust (mine has survived a few drops). All this makes it a great
prototyping board. 

Unfortunately I have found that the PyBadge is not Arduino-friendly. At least not friendly
enough to my liking. It is certainly possible to develop in Arduino but I had to reset
the board before every upload… which is a pain. So CircuitPython it was, plus Python
is a great tool for rapid prototyping.

Unfortunately CircuitPython cannot handle RTC interrupts (according to the current FAQ)
or allow the board to go to sleep, which makes it power hungry for a kitchen timer.

### PyBadge or PyBadge LC

I used a plain PyBadge but the PyBadge LC should work also it is less expensive.
The PyBadge LC has only one NeoPixel (adjust `NEOPIXEL_LEN`) and is limited to the onboard
speaker.

## Libraries
The following [libraries](https://circuitpython.org/libraries) are also used:
* adafruit_bitmap_font
* adafruit_display_text

The font [Noto](https://www.google.com/get/noto/) is from Google. I retrieved
the BDF file from the
[PyBadge Conference Badge project](https://github.com/adafruit/Adafruit_Learning_System_Guides)

## How to use?
At launch, the board shows the configuration screen. Set any of the 3 timers with
the d-pad. Timers you don't need can be left to zero and they will not trigger an alarm.
To start the countdown, hit the _Start_ button.

While the timer is counting down, the _Select_ button returns to the configuration screen.

When one of the timers reaches zero, the alarm goes off. Pressing the _A_ (or _B_) button
or momentarily obstructing the light sensor stops the alarm. If more than one timer has
been selected, the alarm does not stop them. The retimer also restarts automatically,
(unless it was set to zero).

The _B_ button behaves differently for the retimer. It stops the retimer, so for timer 1
and timer 2, you can use either button indifferently. For the retimer button A stops
the alarm but restart the timer. Button B stops the alarm and returns to the configuration
screen.

## A good trade-off
Using a generic board, like the PyBadge, has many benefits in terms of "time to market"
but it's a little bit of a trade-off.

I would be interested to try rotary encoders as an alternative to the d-pad and
the start/stop buttons. A gesture sensor, such as the APDS-9960, would expand options
for hands-free operations, always a good thing in the kitchen.

It would be interested to connect to a thermometer or another sensor, maybe through
Bluetooth. And I have already discussed power saving.

Yet overal I'm happy with the trade-off. At minimum, I have a prototype to validate
in the kitchen. Later I'll see whether I need to improve.
