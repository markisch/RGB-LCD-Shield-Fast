/*!
 * @file Adafruit_RGBLCDShield.cpp
 *
 * @mainpage Adafruit RGB LCD Shield Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the Adafruit RGB 16x2 LCD Shield
 * Pick one up at the Adafruit shop!
 * ---------> http://www.adafruit.com/products/714
 *
 * The shield uses I2C to communicate, 2 pins are required to
 * interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 */

#include "Adafruit_RGBLCDShield_Fast.h"

#include <Wire.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#ifdef __SAM3X8E__ // Arduino Due
#define WIRE Wire1
#else
#define WIRE Wire //!< Specifies which name to use for the I2C bus
#endif

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// RGBLCDShield constructor is called).

Adafruit_RGBLCDShield::Adafruit_RGBLCDShield() {
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  // we can't begin() yet :(
}


void Adafruit_RGBLCDShield::begin(uint8_t cols, uint8_t lines,
                                  uint8_t dotsize) {
  WIRE.begin();
  _i2c.begin();

  // enable burst writes by disabling address increment
  _i2c.byteMode();

  _i2c.pinMode(8, OUTPUT);
  _i2c.pinMode(6, OUTPUT);
  _i2c.pinMode(7, OUTPUT);
  setBacklight(0x7);

  _i2c.pinMode(_rw_pin, OUTPUT);
  _i2c.pinMode(_rs_pin, OUTPUT);
  _i2c.pinMode(_enable_pin, OUTPUT);
  for (uint8_t i = 0; i < 4; i++)
    _i2c.pinMode(_data_pins[i], OUTPUT);

  for (uint8_t i = 0; i < 5; i++) {
    _i2c.pinMode(_button_pins[i], INPUT);
    _i2c.pullUp(_button_pins[i], 1);
  }

  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;
  _currline = 0;

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != 0) && (lines == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way before 4.5V so we'll wait
  // 50
  delayMicroseconds(50000);
  // Now we pull both RS and R/W low to begin commands
  _digitalWrite(_rs_pin, LOW);
  _digitalWrite(_enable_pin, LOW);
  _digitalWrite(_rw_pin, LOW);
  _rw_state = _enable_state = _rs_state = LOW;

  // put the LCD into 4 bit mode
  // this is according to the Hitachi HD44780 datasheet
  // figure 24, pg 46

  // we start in 8bit mode, try to set 4 bit mode
  write4bits(0x03);
  delayMicroseconds(4500); // wait min 4.1ms

  // second try
  write4bits(0x03);
  delayMicroseconds(4500); // wait min 4.1ms

  // third go!
  write4bits(0x03);
  delayMicroseconds(150);

  // finally, set to 8-bit interface
  write4bits(0x02);

  // finally, set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for roman languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);
}

/********** high level commands, for the user! */
void Adafruit_RGBLCDShield::clear() {
  command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
  delayMicroseconds(2000);   // this command takes a long time!
}

void Adafruit_RGBLCDShield::home() {
  command(LCD_RETURNHOME); // set cursor position to zero
  delayMicroseconds(2000); // this command takes a long time!
}

void Adafruit_RGBLCDShield::setCursor(uint8_t col, uint8_t row) {
  int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  if (row > _numlines) {
    row = _numlines - 1; // we count rows starting w/0
  }

  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void Adafruit_RGBLCDShield::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void Adafruit_RGBLCDShield::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void Adafruit_RGBLCDShield::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void Adafruit_RGBLCDShield::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void Adafruit_RGBLCDShield::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void Adafruit_RGBLCDShield::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void Adafruit_RGBLCDShield::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void Adafruit_RGBLCDShield::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void Adafruit_RGBLCDShield::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void Adafruit_RGBLCDShield::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void Adafruit_RGBLCDShield::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void Adafruit_RGBLCDShield::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void Adafruit_RGBLCDShield::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i = 0; i < 8; i++) {
    write(charmap[i]);
  }
  command(LCD_SETDDRAMADDR); // unfortunately resets the location to 0,0
}

/*********** mid level commands, for sending data/cmds */

inline void Adafruit_RGBLCDShield::command(uint8_t value) { send(value, LOW); }

#if ARDUINO >= 100
inline size_t Adafruit_RGBLCDShield::write(uint8_t value) {
  send(value, HIGH);
  return 1;
}
#else
inline void Adafruit_RGBLCDShield::write(uint8_t value) { send(value, HIGH); }
#endif

size_t Adafruit_RGBLCDShield::write(const uint8_t *buffer, size_t size) {
  size_t n = size;
  uint8_t out, out1;
  uint8_t c = 0;

  _rs_state = HIGH;
  _rw_state = LOW;

  // all LCD pins are on port B and we know them all already
  out = ~(_backlight >> 2) & 0x1;
  out |= 0x80;  // RS==HIGH
  out1 = out;

  while (size--) {
    byte value = *buffer++;

    out = out1;
    if (value & 0x10) out |= 0x10;
    if (value & 0x20) out |= 0x08;
    if (value & 0x40) out |= 0x04;
    if (value & 0x80) out |= 0x02;

    // pulse enable
    // _i2c.writeGPIOB(out | 0x20);
    // _i2c.writeGPIOB(out);
    if (c == 0) {
      Wire.beginTransmission(MCP23017_ADDRESS);
      Wire.write(MCP23017_GPIOB);
    }
    Wire.write(out | 0x20);
    Wire.write(out | 0x20);
    Wire.write(out);
    Wire.write(out);

    out = out1;
    if (value & 0x01) out |= 0x10;
    if (value & 0x02) out |= 0x08;
    if (value & 0x04) out |= 0x04;
    if (value & 0x08) out |= 0x02;

    // pulse enable
    // _i2c.writeGPIOB(out | 0x20);
    // _i2c.writeGPIOB(out);   
    Wire.write(out | 0x20);
    Wire.write(out | 0x20);
    Wire.write(out);
    c += 8;
    if (c <= BUFFER_LENGTH - 8) {
      Wire.write(out);
    } else {
      // We only restart the transmission once the buffer is full.
      // We don't need to repeat the last transmitted byte this time.
      Wire.endTransmission();
      c = 0;
    }
  }
  if (c != 0) Wire.endTransmission();
  return n;
}


/************ low level data pushing commands **********/

// little wrapper for i/o writes
void Adafruit_RGBLCDShield::_digitalWrite(uint8_t p, uint8_t d) {
  // an i2c command
  _i2c.digitalWrite(p, d);
}

// Allows to set the backlight, if the LCD backpack is used
void Adafruit_RGBLCDShield::setBacklight(uint8_t status) {
  // check if i2c or SPI
  _i2c.digitalWrite(8, ~(status >> 2) & 0x1);
  _i2c.digitalWrite(7, ~(status >> 1) & 0x1);
  _i2c.digitalWrite(6, ~status & 0x1);
  _backlight = status;
}

// little wrapper for i/o directions
void Adafruit_RGBLCDShield::_pinMode(uint8_t p, uint8_t d) {
  // an i2c command
  _i2c.pinMode(p, d);
}

// write either command or data, with automatic 4/8-bit selection
void Adafruit_RGBLCDShield::send(uint8_t value, uint8_t mode) {
  uint8_t out, out1;

  _rs_state = mode;
  _rw_state = LOW;

  // all LCD pins are on port B and we know them all already
  out = ~(_backlight >> 2) & 0x1;
  if (_rs_state == HIGH)
    out |= 0x80;

  out1 = out;
  if (value & 0x10) out |= 0x10;
  if (value & 0x20) out |= 0x08;
  if (value & 0x40) out |= 0x04;
  if (value & 0x80) out |= 0x02;

  // pulse enable
  // _i2c.writeGPIOB(out | 0x20);
  // _i2c.writeGPIOB(out);
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_GPIOB);
  Wire.write(out | 0x20);
  Wire.write(out | 0x20);
  Wire.write(out);
  Wire.write(out);

  out = out1;
  if (value & 0x01) out |= 0x10;
  if (value & 0x02) out |= 0x08;
  if (value & 0x04) out |= 0x04;
  if (value & 0x08) out |= 0x02;

  // pulse enable
  // _i2c.writeGPIOB(out | 0x20);
  // _i2c.writeGPIOB(out);
  Wire.write(out | 0x20);
  Wire.write(out | 0x20);
  Wire.write(out);
  // We don't need to repeat the last one
  // Wire.write(out);
  Wire.endTransmission();
}

void Adafruit_RGBLCDShield::write4bits(uint8_t value) {
  uint8_t out;

  // all LCD pins are on port B and we know them all already
  out = ~(_backlight >> 2) & 0x1;
  if (_rs_state == HIGH)
    out |= (0x1 << (_rs_pin - 8));
  if (_rw_state == HIGH)
    out |= (0x1 << (_rw_pin - 8));
  for (int i = 0; i < 4; i++)
    out |= ((value >> i) & 0x1) << (_data_pins[i] - 8);

  // make sure enable is low
  if (_enable_state == HIGH) {
    out &= ~(1 << (_enable_pin - 8));
    _i2c.writeGPIOB(out);
    //delayMicroseconds(1);
  }

  // pulse enable
  out |= (1 << (_enable_pin - 8));
  _i2c.writeGPIOB(out);
  //delayMicroseconds(1);
#if 0
  unsigned long time = micros();
  out &= ~(1 << (_enable_pin - 8));
  _i2c.writeGPIOB(out);
  // a single write should take 50us even at 400kHz 
  // we hence shorten the wait time
  //delayMicroseconds(100);
  unsigned long now = micros();
  if ((now - time) < 100)
    delayMicroseconds(100 - (now - time));
#else
  out &= ~(1 << (_enable_pin - 8));
  _i2c.writeGPIOB(out);
#endif
}

uint8_t Adafruit_RGBLCDShield::readButtons(void) {
  // all buttons are on port A: read all in one go
  return (~_i2c.readGPIOA() & 0x1F);
}
