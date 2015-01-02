Remote Controlled LED Lamp
==========================

[Demo Video](https://www.youtube.com/watch?v=SNaRzPEzEGE):

[![Demo Video](http://img.youtube.com/vi/SNaRzPEzEGE/0.jpg)](http://www.youtube.com/watch?v=SNaRzPEzEGE)

I picked up a cheap HOLMÖ Lamp at IKEA, and wired up 64 APA102 LEDs, a Teensy 3.1, 
and an infrared remote receiver.  I added a few patterns from FastLED community members.

Features
--------
* Infrared remote controls all functions/features
* Turn the lamp on and off (the power switch also works, of course).
* Brightness adjustment
* Pattern selection
* 6 patterns:
	* Color Twinkles
	* Hue Cycle (Rainbow)
	* Fire
	* QuadWave (Knight Rider)
	* Bouncing Balls
	* Lightning
* Adjustable solid colors

Hardware
--------
* Lamp: http://www.ikea.com/us/en/catalog/products/30184173/
* APA102 LEDs: https://www.adafruit.com/products/2240
* Teensy 3.1: https://www.pjrc.com/store/teensy31.html or https://www.adafruit.com/product/1625
* Remote: http://www.amazon.com/gp/product/B004IJFAMW/ref=oh_aui_detailpage_o02_s00?ie=UTF8&psc=1 or https://www.adafruit.com/product/389
* IR sensor: https://www.adafruit.com/products/157

Software
--------
* Arduino and Teensyduino: https://www.pjrc.com/teensy/td_download.html
* FastLED: https://github.com/FastLED/FastLED
* IRremote: https://github.com/shirriff/Arduino-IRremote
* Color Twinkles by Mark Kriegsman: https://gist.github.com/kriegsman/5408ecd397744ba0393e
* Fire2012 with programmable Color Palette by Mark Kriegsman: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino
* BouncingBalls2014 by Daniel Wilson: https://github.com/fibonacci162/LEDs/blob/master/BouncingBalls2014/BouncingBalls2014.ino
* Lightning2014 by Daniel Wilson: https://github.com/fibonacci162/LEDs/blob/master/Lightning2014/Lightning2014.ino