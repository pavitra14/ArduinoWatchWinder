#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 

class OledController {
public:
  OledController(uint8_t address = 0x3C);
  bool begin();
  void clear();
  // Displays status, direction, and rotations (tenths precision)
  void updateDashboard(const char *statusText, const char *directionText, int32_t rotationsTenth);
  void showMessage(const char *msg);

private:
  uint8_t _address;
  Adafruit_SH1106G _display;
};
