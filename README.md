This is a fork of the original library for the Adafruit RGB 16x2 LCD Shield.  This shield uses a powerful MCP23017 I2C GPIO-expander.  But the original library is ludicrously slow.

This fork uses optimized I2C access to obtain a more than 10-fold speed increase:  
* Registers are never read back. We cache the state of IO pins instead.
* We remove superfluous delay()s: I2C access is taking care of these already.
* We only do 8bit instead of 16bit writes.
* We switch off the IO expander's automatic address increment to repeatedly update the IO register in a single I2C transaction.
The original code is pretty flexible wrt. to the pin assignments. In order to speed things up, this forks relies on the hardware layout.

Note that you can increase the I2C clock speed using `Wire.setClock(freq)`, or by setting the `TWBR`register directly. My display still works great at `TWBR = 5` with an Arduino UNO, resulting in a 50-fold speed increase compared to the original library with default I2C clock speed.

<hr>

Pick one up at the Adafruit shop!
  ---------> https://www.adafruit.com/products/714

<hr>

This Adafruit shield makes it easy to use a 16x2 Character LCD. We really like the range of LCDs we stock in the shop, such as our classic <a href="http://www.adafruit.com/products/181">blue & white</a> as well as the fancy <a href="http://www.adafruit.com/products/399">RGB negative</a> and <a href="http://www.adafruit.com/products/398">RGB positive</a>. Unfortunately, these LCDs do require quite a few digital pins, 6 to control the LCD and then perhaps another 3 to control the RGB backlight for a total of 9 pins. That's half of the pins available on a classic Arduino! 

With this in mind, we wanted to make it easier for people to get these LCD into their projects so we devised a shield that lets you control <b>a 16x2 Character LCD, up to 3 backlight pins AND 5 keypad pins using only the two I2C pins on the Arduino!</b> The best part is you don't really lose those two pins either, since you can stick i2c-based sensors, RTCs, etc and have them share the I2C bus. This is a super slick way to add a display without all the wiring hassle.

<hr>

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution

<hr>

Additions are copyright by Bastian Maerkisch with the same BSD license.

<hr>

To download. click the DOWNLOADS button in the top right corner, rename the uncompressed folder RGBLCDShield_Fast. Check that the RGBLCDShield_Fast folder contains RGBLCDShield_Fast.cpp and RGBLCDShield_Fast.h

Place the RGBLCDShield_Fast library folder your <arduinosketchfolder>/libraries/ folder. You may need to create the libraries subfolder if its your first library. Restart the IDE.
