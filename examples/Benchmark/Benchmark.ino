// Define exactly one of these to select a specific library:
//#define USE_RGBLCDSHIELD
//#define USE_LIQUIDCRYSTALIO
//#define USE_LIQUIDTWI
#define USE_RGBLCDSHIELDFAST

// Speed-up at 100 kHz (default):
//  Adafruit_RGBLCDShield: 1.0x 
//  LiquidCrystalIO: 0.98x
//  LiquidTWI2: 4.2x
//  RGBShield_Fast: 12.7x

// Speed-up at 400 kHz compared to default speed and original library:
//  Adafruit_RGBLCDShield: 2.6x 
//  LiquidCrystalIO: 2.4x
//  LiquidTWI2: 12.0x
//  RGBShield_Fast: 39.0x

// default I2C clock
//#define I2CLOCK 100000
// I2C fast mode
#define I2CLOCK 400000
// maximum I2C clock for my display (TWBR==5)
//#define I2CLOCK 615384

#if defined(USE_RGBLCDSHIELD) || defined(USE_RGBLCDSHIELDFAST)
# include <Wire.h>
# ifdef USE_RGBLCDSHIELDFAST
#  include <RGBLCDShield_Fast.h>
# else
#  include <Adafruit_RGBLCDShield.h>
#  include <utility/Adafruit_MCP23017.h>
# endif
#endif

#ifdef USE_LIQUIDCRYSTALIO
# include <LiquidCrystalIO.h>
# include <IoAbstractionWire.h>
# include <Wire.h>
#endif

#ifdef USE_LIQUIDTWI
# include <Wire.h>
# include <LiquidTWI2.h>
#endif

#if defined(USE_RGBLCDSHIELDFAST)
// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
RGBLCDShield_Fast lcd = RGBLCDShield_Fast();
#endif

#if defined(USE_RGBLCDSHIELD)
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
#endif

#ifdef USE_LIQUIDCRYSTALIO
const int rs = 15, rw = 14, en = 13, d4 = 12, d5 = 11, d6 = 10, d7 = 9;
LiquidCrystal lcd(rs, rw, en, d4, d5, d6, d7, ioFrom23017(0x20));
#endif

#ifdef USE_LIQUIDTWI
LiquidTWI2 lcd(MCP23017_ADDRESS);
#endif

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

const int nRows = 2;      //number of rows on LCD
const int nColumns = 16;  //number of columns

const int length = nRows * nColumns;
char text[length+1];
char blanks[length+1];

void setup(void) {

  Serial.begin(57600);

#ifdef USE_LIQUIDCRYSTALIO
  Wire.begin();
  lcd.begin(nColumns,nRows);
  lcd.configureBacklightPin(7);
  lcd.backlight();
  //lcd.setDelayTime(0x00, 0);
#endif

#if defined(USE_RGBLCDSHIELD) || defined(USE_RGBLCDSHIELDFAST) || defined(USE_LIQUIDTWI)
  Wire.begin();
  lcd.begin(nColumns,nRows);
  lcd.setBacklight(WHITE);
#endif

  // First speed tweak: increase I2C Clock to 400kHz (full speed)
  // Note that doing that _before_ lcd.begin() does not work since it will
  // reset to defaults.
  // Speed: 7281 ms -> 2733 ms
  // Wire.setClock(400000);
  Wire.setClock(I2CLOCK);
  //TWBR = 0x05;

  Serial.print(F("TWBR: "));
  Serial.println(TWBR, 16);
  Serial.println(F_CPU / (16 + (2 * TWBR)));

  lcd.clear();
  lcd.setCursor(0,0);
  char c = 'A';
  for (int i=0; i<length; i++) {
    text[i] = c++;
    blanks[i] = ' ';
    if (c > 'Z') c = 'A';
  }
  text[length] = 0;
  blanks[length] = 0;
  unsigned long startTime=millis();
  byte repetitions = 20;
  while (repetitions--) {
    lcd.setCursor(0,0);  // fill every screen pixel with text
    lcd.print(text);
    lcd.setCursor(0,0);  // then fill every pixel with blanks and repeat
    lcd.print(blanks);
  }
  unsigned long endTime = millis();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Benchmark "));
  lcd.print(nColumns, DEC);
  lcd.write('x');
  lcd.print(nRows, DEC);
  lcd.setCursor(0,1);
  lcd.print(endTime - startTime);
  lcd.print(F(" ms,  "));
  unsigned long x = (740900 / (endTime - startTime) + 5) / 10;
  if (x < 100)
    lcd.write(' ');
  lcd.print(x / 10);
  lcd.write('.');
  lcd.print(x % 10);
  lcd.write('x');
  //Serial.println(endTime - startTime);
  //Serial.println(x);
}

void loop() {
}
