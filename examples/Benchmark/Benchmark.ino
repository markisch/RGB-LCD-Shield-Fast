#include <Wire.h>
#include <Adafruit_RGBLCDShield_Fast.h>
//#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

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
  lcd.begin(nColumns,nRows);

  // First speed tweak: increase I2C Clock to 400kHz (full speed)
  // Note that doing that _before_ lcd.begin() does not work since it will
  // reset to defaults.
  // Speed: 7281 ms -> 2733 ms
  Wire.setClock(400000);

  lcd.setBacklight(WHITE);
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
  unsigned long x = (728100 / (endTime - startTime) + 5) / 10;
  if (x < 100)
    lcd.write(' ');
  lcd.print(x / 10);
  lcd.write('.');
  lcd.print(x % 10);
  lcd.write('x');
}

void loop() {
}
