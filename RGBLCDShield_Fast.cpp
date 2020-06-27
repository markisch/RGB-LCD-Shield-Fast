/*!
 * @file RGBLCDShield_Fast.cpp
 *
 * @mainpage Fork of Adafruit RGB LCD Shield Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the Adafruit RGB 16x2 LCD Shield
 * Pick one up at the Adafruit shop!
 * ---------> http://www.adafruit.com/products/714
 *
 * The shield uses I2C to communicate, 2 pins are required to
 * interface. This fork is significantly faster than the original.
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * Modified by Bastian Maerkisch.
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 */

#include "RGBLCDShield_Fast.h"

#include <Wire.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <compat/twi.h>
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

RGBLCDShield_Fast::RGBLCDShield_Fast() {
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  // we can't begin() yet :(
}


void RGBLCDShield_Fast::begin(uint8_t cols, uint8_t lines,
                                  uint8_t dotsize) {
#ifdef __AVR__
  // Only initialize wire interface if not yet done
  if ((TWCR & _BV(TWEN)) != _BV(TWEN))
#endif
    WIRE.begin();
  _i2c.begin();

  // enable burst writes by disabling address increment (requires bank mode)
  _i2c.burstMode();

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
    _i2c.pullUp(_button_pins[i], HIGH);
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
void RGBLCDShield_Fast::clear() {
  command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
  waitBusy();                // this command takes a long time!
}

void RGBLCDShield_Fast::home() {
  command(LCD_RETURNHOME); // set cursor position to zero
  waitBusy();              // this command takes a long time!
}

void RGBLCDShield_Fast::setCursor(uint8_t col, uint8_t row) {
  int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
  if (row > _numlines) {
    row = _numlines - 1; // we count rows starting w/0
  }

  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void RGBLCDShield_Fast::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void RGBLCDShield_Fast::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void RGBLCDShield_Fast::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void RGBLCDShield_Fast::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void RGBLCDShield_Fast::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void RGBLCDShield_Fast::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void RGBLCDShield_Fast::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void RGBLCDShield_Fast::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void RGBLCDShield_Fast::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}
// This is for text that flows Right to Left
void RGBLCDShield_Fast::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void RGBLCDShield_Fast::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}
// This will 'left justify' text from the cursor
void RGBLCDShield_Fast::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void RGBLCDShield_Fast::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  // Note that this somehow does not work with burst mode:
  // write(charmap, 8);
  for (int i = 0; i < 8; i++) {
    write(charmap[i]);
  }
  command(LCD_SETDDRAMADDR); // unfortunately resets the location to 0,0
}

void RGBLCDShield_Fast::createCharPgm(uint8_t location, const uint8_t *charmapP) {
  PGM_P p = reinterpret_cast<PGM_P>(charmapP);
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    byte c = pgm_read_byte(p++);
    write(c);
  }
  command(LCD_SETDDRAMADDR);  // unfortunately resets the location to 0,0
}

/*********** mid level commands, for sending data/cmds */

inline void RGBLCDShield_Fast::command(uint8_t value) {
  send(value, LOW);
}

#if ARDUINO >= 100
inline size_t RGBLCDShield_Fast::write(uint8_t value) {
  send(value, HIGH);
  return 1;
}
#else
inline void RGBLCDShield_Fast::write(uint8_t value) {
  send(value, HIGH);
}
#endif

size_t RGBLCDShield_Fast::write(const uint8_t *buffer, size_t size) {
  size_t n = size;
  uint8_t out, out1;
  uint8_t c = 0;

  _rs_state = HIGH;
  _rw_state = LOW;

  // all LCD pins are on port B and we know all bits already
  out = ~(_backlight >> 2) & 0x1;
  out |= _rs_mask;  // RS==HIGH
  out1 = out;

  while (size--) {
    byte value = *buffer++;

    out = out1;
    if (value & 0x10) out |= _data_mask[0];
    if (value & 0x20) out |= _data_mask[1];
    if (value & 0x40) out |= _data_mask[2];
    if (value & 0x80) out |= _data_mask[3];

    // pulse enable
    // _i2c.writeGPIOB(out | _enable_mask);
    // _i2c.writeGPIOB(out);
    if (c == 0) {
      Wire.beginTransmission(MCP23017_ADDRESS);
      Wire.write(MCP23017_BANK_GPIOB);
    }
    Wire.write(out | _enable_mask);
    Wire.write(out);

    out = out1;
    if (value & 0x01) out |= _data_mask[0];
    if (value & 0x02) out |= _data_mask[1];
    if (value & 0x04) out |= _data_mask[2];
    if (value & 0x08) out |= _data_mask[3];

    // pulse enable
    // _i2c.writeGPIOB(out | _enable_mask);
    // _i2c.writeGPIOB(out);   
    Wire.write(out | _enable_mask);
    Wire.write(out);
    c += 4;
    if (c >= BUFFER_LENGTH - 4) {
      // We only restart the transmission once the buffer is full.
      Wire.endTransmission();
      c = 0;
    }
  }
  if (c != 0) Wire.endTransmission();
  return n;
}


/************ low level data pushing commands **********/

// little wrapper for i/o writes
void RGBLCDShield_Fast::_digitalWrite(uint8_t p, uint8_t d) {
  // an i2c command
  _i2c.digitalWrite(p, d);
}

// Allows to set the backlight, if the LCD backpack is used
void RGBLCDShield_Fast::setBacklight(uint8_t status) {
  // check if i2c or SPI
  _i2c.digitalWrite(8, ~(status >> 2) & 0x1);
  _i2c.digitalWrite(7, ~(status >> 1) & 0x1);
  _i2c.digitalWrite(6, ~status & 0x1);
  _backlight = status;
}

// little wrapper for i/o directions
void RGBLCDShield_Fast::_pinMode(uint8_t p, uint8_t d) {
  // an i2c command
  _i2c.pinMode(p, d);
}

int RGBLCDShield_Fast::waitBusy() {
  int n = 0;
  _rs_state = LOW;
  _rw_state = HIGH;

  // Set data lines as input
  // for (i = 0; i < 4; i++)
    // pinMode(_data_pins[i], INPUT);
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_BANK_IODIRB);
  Wire.write(0x1e);
  Wire.endTransmission();

  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_BANK_GPIOB);

  const uint8_t out = _rw_mask;
  uint8_t busy;
  do {
    Wire.write(out | _enable_mask);
    Wire.endTransmission();

    // Burst mode. No need to set address again.
    Wire.requestFrom(MCP23017_ADDRESS, 1);
    busy = Wire.read() & _data_mask[3];

    Wire.beginTransmission(MCP23017_ADDRESS);
    Wire.write(MCP23017_BANK_GPIOB);
    Wire.write(out);
    Wire.write(out | _enable_mask);
    Wire.write(out);

    n++;
  } while (busy);

  Wire.endTransmission();

  // for (i = 0; i < 4; i++)
    // pinMode(_data_pins[i], OUTPUT);
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_BANK_IODIRB);
  Wire.write(0x00);  // all output
  Wire.endTransmission();

  return n;
}

// write either command or data, with automatic 4/8-bit selection
void RGBLCDShield_Fast::send(uint8_t value, uint8_t mode) {
  uint8_t out, out1;

  _rs_state = mode;
  _rw_state = LOW;

  // all LCD pins are on port B and we know all bits already
  out = ~(_backlight >> 2) & 0x1;
  if (_rs_state == HIGH)
    out |= _rs_mask;

  out1 = out;
  if (value & 0x10) out |= _data_mask[0];
  if (value & 0x20) out |= _data_mask[1];
  if (value & 0x40) out |= _data_mask[2];
  if (value & 0x80) out |= _data_mask[3];

  // pulse enable
  // _i2c.writeGPIOB(out | _enable_mask);
  // _i2c.writeGPIOB(out);
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_BANK_GPIOB);
  Wire.write(out | _enable_mask);
  Wire.write(out);

  out = out1;
  if (value & 0x01) out |= _data_mask[0];
  if (value & 0x02) out |= _data_mask[1];
  if (value & 0x04) out |= _data_mask[2];
  if (value & 0x08) out |= _data_mask[3];

  // pulse enable
  // _i2c.writeGPIOB(out | _enable_mask);
  // _i2c.writeGPIOB(out);
  Wire.write(out | _enable_mask);
  Wire.write(out);
  Wire.endTransmission();
}

void RGBLCDShield_Fast::write4bits(uint8_t value) {
  uint8_t out;

  // all LCD pins are on port B and we know them all already
  out = ~(_backlight >> 2) & 0x1;
  if (_rs_state == HIGH)
    out |= _rs_mask;
  if (_rw_state == HIGH)
    out |= _rw_mask;
  for (int i = 0; i < 4; i++) {
    if ((value >> i) & 0x1)
      out |= _data_mask[i];
  }

  // make sure enable is low
  if (_enable_state == HIGH) {
    out &= ~_enable_mask;
    _i2c.writeGPIOB(out);
  }

  // pulse enable
  out |= _enable_mask;
  _i2c.writeGPIOB(out);
  out &= ~_enable_mask;
  _i2c.writeGPIOB(out);
}

uint8_t RGBLCDShield_Fast::readButtons(void) {
  // all buttons are on port A: read all in one go
  return (~_i2c.readGPIOA() & 0x1f);
}
