/*********************

Example code for the Adafruit RGB Character LCD Shield and Library

This code displays text on the shield, and also reads the buttons on the keypad.
When a button is pressed, the backlight changes color.

**********************/

// Define if your LCD has a RGB backlight:
// #define RGB_BACKLIGHT

// include the library code:
#include <Wire.h>
#include <RGBLCDShield_Fast.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
RGBLCDShield_Fast lcd = RGBLCDShield_Fast();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

void setup() {
  // Debugging output
  Serial.begin(9600);
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);

  // Print a message to the LCD. We track how long it takes since
  // this library has been optimized a bit and we're proud of it :)
  int time = millis();
  lcd.print(F("Hello, world!"));
  time = millis() - time;
  Serial.print(F("Took ")); Serial.print(time); Serial.println(F(" ms"));
  lcd.setBacklight(WHITE);
}

uint8_t i=0;

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis()/1000);

  uint8_t buttons = lcd.readButtons();

  if (buttons != i) {
    i = buttons;
    lcd.setCursor(0,0);
    lcd.print(F("                "));
    lcd.setCursor(0,0);
    if (buttons & BUTTON_UP) {
      lcd.print(F("UP "));
#ifdef RGB_BACKLIGHT      
      lcd.setBacklight(RED);
#endif
    }
    if (buttons & BUTTON_DOWN) {
      lcd.print(F("DOWN "));
#ifdef RGB_BACKLIGHT      
      lcd.setBacklight(YELLOW);
#endif
    }
    if (buttons & BUTTON_LEFT) {
      lcd.print(F("LEFT "));
#ifdef RGB_BACKLIGHT      
      lcd.setBacklight(GREEN);
#endif
    }
    if (buttons & BUTTON_RIGHT) {
      lcd.print(F("RIGHT "));
#ifdef RGB_BACKLIGHT      
      lcd.setBacklight(TEAL);
#endif
    }
    if (buttons & BUTTON_SELECT) {
      lcd.print(F("SELECT "));
#ifdef RGB_BACKLIGHT      
      lcd.setBacklight(VIOLET);
#endif
    }
    if (buttons == 0) {
      lcd.print(F("(No key)"));
    }
  }
}
