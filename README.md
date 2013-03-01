ShuttAVR
========

AVR-based IR intervalometer for digital cameras.

About
-----

This code allows you to control your camera's shutter via IR. It's main features are:

1. Single-shot shutter control
2. Timer-based (intervalometer) shutter control

The current design works with Nikon cameras only, though it should be easy to
modify for any arbitrary camera.

Usage
-----

ShuttAVR has two modes of operation: single-shot mode and intervalometer mode

### Single-shot mode

- Press the button
- Shutter is fired

### Intervalometer mode

- Hold button for more than 3 seconds
- Release button
- Wait for your desired delay interval
- Press the button to save the delay
- ShuttAVR will fire the shutter at the delay you just set
- Process repeats until button is pressed again

Bill of Materials
-----------------

ShuttAVR was design with the following parts in mind:

- ATtiny25 (aka t25)
- IR LED
- 3V battery supply
- SPST switch
- 1k resistor
- 220 resistor

Note that these parts are not a requirement for this project, only a guideline. Feel free to roll your own BOM.

Pinout
------

See main.c

Building
--------

To build, you will need:

- scons
- avr-toolchain (I recommend CrossPack)
- Programmer* (I used an Arduino Uno loaded with ArduinoISP)
    - *You can feasibly program it by hand, if you have lots of time

Build commands you are interested in:

- `scons`: build project
- `scons flash`: build and flash file using `avrdude` to chip

The current `sconstruct` file is configured for the t25 at 1 MHz. It also does a
bit of magic to find my ArduinoISP programmer. You may need to change these to
get it to work, or just compile the files manually.

Design
------

ShuttAVR's software design is entirely interrupt driven, with the primary
purpose to reduce power consumption. Microcontrollers use far less power when in
a sleep state, so the design attempts to keep the micro asleep as often as it
can.

The t25 only has two timers, so there is a bit of juggling sometimes when going
between delay recording and LED sequencing.

Future Design Considerations
----------------------------

On the t25, there is a deep sleep mode which deactivates all clocks. To wake it
back up, you need to drive a pin low. I would have used this mode for
non-intervalometer mode, but I hooked my switch up to be active high. If you
want to go for really low power, reverse the polarity of the switch, as well as
the edge detection logic in the switch interrupt handler.
