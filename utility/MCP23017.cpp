/***************************************************
  This is a library for the MCP23017 i2c port expander

  These displays use I2C to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************

  Modified by Bastian Maerkisch.  All changes BSD licensed.

 ****************************************************/

#include <Wire.h>
#ifdef __AVR
#include <avr/pgmspace.h>
#elif defined(ESP8266)
#include <pgmspace.h>
#endif
#include "MCP23017.h"
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

MCP23017::MCP23017() {
  mode = -1;  // we just don't know yet
}

void MCP23017::begin(uint8_t addr) {
  if (addr > 7) {
    addr = 7;
  }
  i2caddr = addr;

#ifdef __AVR__
  // Only initialize wire interface if not yet done
  if ((TWCR & _BV(TWEN)) != _BV(TWEN))
#endif
    WIRE.begin();

  // We may be in "burst" or normal mode. Revert to bank=0, seq=0.
  // Assume we are in BANK=1 mode. Read register.
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_BANK_IOCONA);
  WIRE.endTransmission();
  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  uint8_t val = wirerecv();
  // Clear BANK bit. This might also be GPINTENB.GPINT7 if we are in BANK=0 mode.
  val &= 0x7f;
  // Write back to make sure we are in BANK=0 mode
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_BANK_IOCONA);
  wiresend(val);
  WIRE.endTransmission();
  // Finally, we also clear the increment address bit:
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(MCP23017_SEQ_IOCONA);
  wiresend(0x00);
  WIRE.endTransmission();
  // We are in mode == 0 now.
  mode = 0;
  // Update register variables.
  normalMode();

  // set defaults!
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(IODIRA);
  wiresend(0xFF); // all inputs on port A
  WIRE.endTransmission();

  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(IODIRB);
  wiresend(0xFF); // all inputs on port B
  WIRE.endTransmission();
}

void MCP23017::begin(void) {
  begin(0);
}

void MCP23017::pinMode(uint8_t p, uint8_t d) {
  uint8_t iodir;
  uint8_t iodiraddr;

  // only 16 bits!
  if (p > 15)
    return;

  if (p < 8)
    iodiraddr = IODIRA;
  else {
    iodiraddr = IODIRB;
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

uint8_t MCP23017::readGPIOA() {
  // read the current GPIO output latches
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(GPIOA);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  return wirerecv();
}

uint8_t MCP23017::readGPIOB() {
  // read the current GPIO output latches
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(GPIOB);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  return wirerecv();
}

uint16_t MCP23017::readGPIOAB() {
  uint16_t ba = 0;
  uint8_t a;

  // read the current GPIO output latches
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(GPIOA);
  WIRE.endTransmission();

  if (mode == 0) {
    WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 2);
    a = wirerecv();
    ba = wirerecv();
  } else {
    WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
    a = wirerecv();

    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(GPIOB);
    WIRE.endTransmission();
    WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
    ba = wirerecv();
  }
  ba <<= 8;
  ba |= a;

  return ba;
}

void MCP23017::writeGPIOA(uint8_t a) {
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(GPIOA);
  wiresend(a);
  WIRE.endTransmission();
}

void MCP23017::writeGPIOB(uint8_t b) {
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(GPIOB);
  wiresend(b);
  WIRE.endTransmission();
}

void MCP23017::writeGPIOAB(uint16_t ba) {
  if (mode == 0) {
    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(GPIOA);
    wiresend(ba & 0xFF);
    wiresend(ba >> 8);
    WIRE.endTransmission();
  } else {
    writeGPIOA(ba & 0xFF);
    writeGPIOB(ba >> 8);
  }
}

void MCP23017::digitalWrite(uint8_t p, uint8_t d) {
  uint8_t gpio;
  uint8_t gpioaddr, olataddr;

  // only 16 bits!
  if (p > 15)
    return;

  if (p < 8) {
    olataddr = OLATA;
    gpioaddr = GPIOA;
  } else {
    olataddr = OLATB;
    gpioaddr = GPIOB;
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

void MCP23017::pullUp(uint8_t p, uint8_t d) {
  uint8_t gppu;
  uint8_t gppuaddr;

  // only 16 bits!
  if (p > 15)
    return;

  if (p < 8)
    gppuaddr = GPPUA;
  else {
    gppuaddr = GPPUB;
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

uint8_t MCP23017::digitalRead(uint8_t p) {
  uint8_t gpioaddr;

  // only 16 bits!
  if (p > 15)
    return 0;

  if (p < 8)
    gpioaddr = GPIOA;
  else {
    gpioaddr = GPIOB;
    p -= 8;
  }

  // read the current GPIO
  WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
  wiresend(gpioaddr);
  WIRE.endTransmission();

  WIRE.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
  return (wirerecv() >> p) & 0x1;
}

void MCP23017::normalMode() {
  // We turn on sequential mode and disable banking.
  if (mode != 0) {
    // First, we assume we are in BANK=1 mode.
    // Caution: this changes all register locations, including the IOCON!
    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(MCP23017_BANK_IOCONA);  // IOCON in BANK=1 mode
    wiresend(0x00);  // BANK=1, SEQOP=0
    WIRE.endTransmission();

    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(MCP23017_BANK_IOCONB);
    wiresend(0x00);
    WIRE.endTransmission();
  }
  mode = 0;

  // update register addresses
  IODIRA = MCP23017_SEQ_IODIRA;
  IPOLA = MCP23017_SEQ_IPOLA;
  GPINTENA = MCP23017_SEQ_GPINTENA;
  DEFVALA = MCP23017_SEQ_DEFVALA;
  INTCONA = MCP23017_SEQ_INTCONA;
  IOCONA = MCP23017_SEQ_IOCONA;
  GPPUA = MCP23017_SEQ_GPPUA;
  INTFA = MCP23017_SEQ_INTFA;
  INTCAPA = MCP23017_SEQ_INTCAPA;
  GPIOA = MCP23017_SEQ_GPIOA;
  OLATA = MCP23017_SEQ_OLATA;

  IODIRB = MCP23017_SEQ_IODIRB;
  IPOLB = MCP23017_SEQ_IPOLB;
  GPINTENB = MCP23017_SEQ_GPINTENB;
  DEFVALB = MCP23017_SEQ_DEFVALB;
  INTCONB = MCP23017_SEQ_INTCONB;
  IOCONB = MCP23017_SEQ_IOCONB;
  GPPUB = MCP23017_SEQ_GPPUB;
  INTFB = MCP23017_SEQ_INTFB;
  INTCAPB = MCP23017_SEQ_INTCAPB;
  GPIOB = MCP23017_SEQ_GPIOB;
  OLATB = MCP23017_SEQ_OLATB;
}

void MCP23017::burstMode() {
  // Turn off sequential mode and activate banking.
  // This allows repeated access to the same register within a single I2C transition.

  if (mode != 1) {
    // First, we assume we are in BANK=0 mode.
    // Caution: this changes all register locations, including the IOCON!
    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(MCP23017_SEQ_IOCONA);  // IOCON in BANK=0 mode
    wiresend(0x80);  // BANK=1, SEQOP=0
    WIRE.endTransmission();
/*
    // If we were already in BANK=1 mode, we reset register OLATA to zero.
    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(MCP23017_SEQ_OLATA);  // hard-code register address: OLATA in BANK=1 mode
    wiresend(0x00);  // zero output latch
    WIRE.endTransmission();
*/
    // Make sure we are in non-sequential mode
    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(MCP23017_BANK_IOCONA);
    wiresend(0x80 | 0x20);  // BANK=1, SEQOP=1 - Byte mode w/o sequential addressing
    WIRE.endTransmission();
/*
    WIRE.beginTransmission(MCP23017_ADDRESS | i2caddr);
    wiresend(MCP23017_BANK_IOCONB);
    wiresend(0x80 | 0x20);
    WIRE.endTransmission();
*/
  }
  mode = 1;

  // update register addresses
  IODIRA = MCP23017_BANK_IODIRA;
  IPOLA = MCP23017_BANK_IPOLA;
  GPINTENA = MCP23017_BANK_GPINTENA;
  DEFVALA = MCP23017_BANK_DEFVALA;
  INTCONA = MCP23017_BANK_INTCONA;
  IOCONA = MCP23017_BANK_IOCONA;
  GPPUA = MCP23017_BANK_GPPUA;
  INTFA = MCP23017_BANK_INTFA;
  INTCAPA = MCP23017_BANK_INTCAPA;
  GPIOA = MCP23017_BANK_GPIOA;
  OLATA = MCP23017_BANK_OLATA;

  IODIRB = MCP23017_BANK_IODIRB;
  IPOLB = MCP23017_BANK_IPOLB;
  GPINTENB = MCP23017_BANK_GPINTENB;
  DEFVALB = MCP23017_BANK_DEFVALB;
  INTCONB = MCP23017_BANK_INTCONB;
  IOCONB = MCP23017_BANK_IOCONB;
  GPPUB = MCP23017_BANK_GPPUB;
  INTFB = MCP23017_BANK_INTFB;
  INTCAPB = MCP23017_BANK_INTCAPB;
  GPIOB = MCP23017_BANK_GPIOB;
  OLATB = MCP23017_BANK_OLATB;
}
