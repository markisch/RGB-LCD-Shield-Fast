/***************************************************
  This is a library for the MCP23017 i2c port expander

  These displays use I2C to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <Wire.h>
#ifdef __AVR
#include <avr/pgmspace.h>
#elif defined(ESP8266)
#include <pgmspace.h>
#endif
#include "Adafruit_MCP23017.h"
#ifdef __SAM3X8E__ // Arduino Due
#define WIRE Wire1
#else
#define WIRE Wire
#endif

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// minihelper
static inline void wiresend(uint8_t x) {
#if ARDUINO >= 100
  WIRE.write((uint8_t)x);
#else
  WIRE.send(x);
#endif
}

static inline uint8_t wirerecv(void) {
#if ARDUINO >= 100
  return WIRE.read();
#else
  return WIRE.receive();
#endif
}

////////////////////////////////////////////////////////////////////////////////

void Adafruit_MCP23017::begin(uint8_t addr) {
  if (addr > 7) {
    addr = 7;
  }
  i2caddr = addr;

  // Only initialize wire interface if not yet done
  if ((TWCR & _BV(TWEN)) != _BV(TWEN))
    WIRE.begin();

  // Caution: this changes all register locations, including the IOCON!
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(0x0A);  // hard-code register address:  IOCON in BANK=0 mode
  wiresend(0x80);  // BANK=1, SEQOP=1 - Byte mode w/o sequential addressing
  WIRE.endTransmission();

  // If we were already in BANK=1 mode, we reset register OLATA to zero
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(0x0A);  // hard-code register address: OLATA in BANK=1 mode
  wiresend(0x00);  // zero output latch
  WIRE.endTransmission();

  // Make sure we are in non-sequential mode
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_IOCONA);
  wiresend(0x80 | 0x20);  // BANK=1, SEQOP=1 - Byte mode w/o sequential addressing
  WIRE.endTransmission();

  // set defaults!
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_IODIRA);
  wiresend(0xFF); // all inputs on port A
  WIRE.endTransmission();

  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_IODIRB);
  wiresend(0xFF); // all inputs on port B
  WIRE.endTransmission();
}

void Adafruit_MCP23017::begin(void) { begin(0); }

void Adafruit_MCP23017::pinMode(uint8_t p, uint8_t d) {
  uint8_t iodir;
  uint8_t iodiraddr;

  // only 16 bits!
  if (p > 15)
    return;

  if (p < 8)
    iodiraddr = MCP23017_IODIRA;
  else {
    iodiraddr = MCP23017_IODIRB;
    p -= 8;
  }

  // read the current IODIR
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(iodiraddr);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  iodir = wirerecv();

  // set the pin and direction
  if (d == INPUT) {
    iodir |= 1 << p;
  } else {
    iodir &= ~(1 << p);
  }

  // write the new IODIR
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(iodiraddr);
  wiresend(iodir);
  WIRE.endTransmission();
}

uint8_t Adafruit_MCP23017::readGPIOA() {
  // read the current GPIO output latches
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_GPIOA);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  return wirerecv();
}

uint8_t Adafruit_MCP23017::readGPIOB() {
  // read the current GPIO output latches
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_GPIOB);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  return wirerecv();
}

uint16_t Adafruit_MCP23017::readGPIOAB() {
  uint16_t ba = 0;
  uint8_t a;

  // read the current GPIO output latches
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_GPIOA);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 2);
  a = wirerecv();
  ba = wirerecv();
  ba <<= 8;
  ba |= a;

  return ba;
}

void Adafruit_MCP23017::writeGPIOA(uint8_t a) {
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_GPIOA);
  wiresend(a);
  WIRE.endTransmission();
}

void Adafruit_MCP23017::writeGPIOB(uint8_t b) {
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_GPIOB);
  wiresend(b);
  WIRE.endTransmission();
}

void Adafruit_MCP23017::writeGPIOAB(uint16_t ba) {
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_GPIOA);
  wiresend(ba & 0xFF);
  wiresend(ba >> 8);
  WIRE.endTransmission();
}

void Adafruit_MCP23017::digitalWrite(uint8_t p, uint8_t d) {
  uint8_t gpio;
  uint8_t gpioaddr, olataddr;

  // only 16 bits!
  if (p > 15)
    return;

  if (p < 8) {
    olataddr = MCP23017_OLATA;
    gpioaddr = MCP23017_GPIOA;
  } else {
    olataddr = MCP23017_OLATB;
    gpioaddr = MCP23017_GPIOB;
    p -= 8;
  }

  // read the current GPIO output latches
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(olataddr);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  gpio = wirerecv();

  // set the pin and direction
  if (d == HIGH) {
    gpio |= 1 << p;
  } else {
    gpio &= ~(1 << p);
  }

  // write the new GPIO
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(gpioaddr);
  wiresend(gpio);
  WIRE.endTransmission();
}

void Adafruit_MCP23017::pullUp(uint8_t p, uint8_t d) {
  uint8_t gppu;
  uint8_t gppuaddr;

  // only 16 bits!
  if (p > 15)
    return;

  if (p < 8)
    gppuaddr = MCP23017_GPPUA;
  else {
    gppuaddr = MCP23017_GPPUB;
    p -= 8;
  }

  // read the current pullup resistor set
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(gppuaddr);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  gppu = wirerecv();

  // set the pin and direction
  if (d == HIGH) {
    gppu |= 1 << p;
  } else {
    gppu &= ~(1 << p);
  }

  // write the new GPIO
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(gppuaddr);
  wiresend(gppu);
  WIRE.endTransmission();
}

uint8_t Adafruit_MCP23017::digitalRead(uint8_t p) {
  uint8_t gpioaddr;

  // only 16 bits!
  if (p > 15)
    return 0;

  if (p < 8)
    gpioaddr = MCP23017_GPIOA;
  else {
    gpioaddr = MCP23017_GPIOB;
    p -= 8;
  }

  // read the current GPIO
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(gpioaddr);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  return (wirerecv() >> p) & 0x1;
}

void Adafruit_MCP23017::sequentialMode()
{
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_IOCONB);
  wiresend(0x80);
  WIRE.endTransmission();
}

void Adafruit_MCP23017::byteMode()
{
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_IOCONB);
  wiresend(0x80 | 0x20);
  WIRE.endTransmission();
}
