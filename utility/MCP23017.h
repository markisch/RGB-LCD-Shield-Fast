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

#ifndef _MCP23017_H_
#define _MCP23017_H_

// Don't forget the Wire library
class MCP23017 {
public:
  MCP23017();

  void begin(uint8_t addr);
  void begin(void);

  void pinMode(uint8_t p, uint8_t d);
  void digitalWrite(uint8_t p, uint8_t d);
  void pullUp(uint8_t p, uint8_t d);
  uint8_t digitalRead(uint8_t p);

  void writeGPIOA(uint8_t);
  void writeGPIOB(uint8_t);
  void writeGPIOAB(uint16_t);
  uint8_t readGPIOA();
  uint8_t readGPIOB();
  uint16_t readGPIOAB();

  void normalMode();
  void burstMode();

  uint8_t readRegister(uint8_t);
  void writeRegister(uint8_t, uint8_t);
  void updateRegister(uint8_t, uint8_t, bool);

private:
  uint8_t i2caddr;
  uint8_t mode;  // mode == 0:  auto-increment address, non-banked
                 // mode == 1:  "burst": non address increment, banked register addresses

public:
  uint8_t IODIRA;
  uint8_t IPOLA;
  uint8_t GPINTENA;
  uint8_t DEFVALA;
  uint8_t INTCONA;
  uint8_t IOCONA;
  uint8_t GPPUA;
  uint8_t INTFA;
  uint8_t INTCAPA;
  uint8_t GPIOA;
  uint8_t OLATA;

  uint8_t IODIRB;
  uint8_t IPOLB;
  uint8_t GPINTENB;
  uint8_t DEFVALB;
  uint8_t INTCONB;
  uint8_t IOCONB;
  uint8_t GPPUB;
  uint8_t INTFB;
  uint8_t INTCAPB;
  uint8_t GPIOB;
  uint8_t OLATB;
};

#define MCP23017_ADDRESS 0x20

// registers, ICON.BANK == 0
#define MCP23017_SEQ_IODIRA 0x00
#define MCP23017_SEQ_IPOLA 0x02
#define MCP23017_SEQ_GPINTENA 0x04
#define MCP23017_SEQ_DEFVALA 0x06
#define MCP23017_SEQ_INTCONA 0x08
#define MCP23017_SEQ_IOCONA 0x0A
#define MCP23017_SEQ_GPPUA 0x0C
#define MCP23017_SEQ_INTFA 0x0E
#define MCP23017_SEQ_INTCAPA 0x10
#define MCP23017_SEQ_GPIOA 0x12
#define MCP23017_SEQ_OLATA 0x14

#define MCP23017_SEQ_IODIRB 0x01
#define MCP23017_SEQ_IPOLB 0x03
#define MCP23017_SEQ_GPINTENB 0x05
#define MCP23017_SEQ_DEFVALB 0x07
#define MCP23017_SEQ_INTCONB 0x09
#define MCP23017_SEQ_IOCONB 0x0B
#define MCP23017_SEQ_GPPUB 0x0D
#define MCP23017_SEQ_INTFB 0x0F
#define MCP23017_SEQ_INTCAPB 0x11
#define MCP23017_SEQ_GPIOB 0x13
#define MCP23017_SEQ_OLATB 0x15

// registers, ICON.BANK == 1
#define MCP23017_BANK_IODIRA 0x00
#define MCP23017_BANK_IPOLA 0x01
#define MCP23017_BANK_GPINTENA 0x02
#define MCP23017_BANK_DEFVALA 0x03
#define MCP23017_BANK_INTCONA 0x04
#define MCP23017_BANK_IOCONA 0x05
#define MCP23017_BANK_GPPUA 0x06
#define MCP23017_BANK_INTFA 0x07
#define MCP23017_BANK_INTCAPA 0x08
#define MCP23017_BANK_GPIOA 0x09
#define MCP23017_BANK_OLATA 0x0A

#define MCP23017_BANK_IODIRB 0x10
#define MCP23017_BANK_IPOLB 0x11
#define MCP23017_BANK_GPINTENB 0x12
#define MCP23017_BANK_DEFVALB 0x13
#define MCP23017_BANK_INTCONB 0x14
#define MCP23017_BANK_IOCONB 0x15
#define MCP23017_BANK_GPPUB 0x16
#define MCP23017_BANK_INTFB 0x17
#define MCP23017_BANK_INTCAPB 0x18
#define MCP23017_BANK_GPIOB 0x19
#define MCP23017_BANK_OLATB 0x1A

#endif
