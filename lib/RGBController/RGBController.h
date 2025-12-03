#pragma once
#include <Arduino.h>

class RGBController {
public:
  RGBController(uint8_t rPin, uint8_t gPin, uint8_t bPin);
  void begin();
  void setColor(uint8_t r, uint8_t g, uint8_t b);
  void setOn(bool on);
  void setBrightness(uint8_t b); // 0..255
  // convenience
  void setRed();
  void setGreen();
  void setOff();
private:
  uint8_t rPin,gPin,bPin;
  uint8_t curR,curG,curB;
  uint8_t brightness = 200;
  bool isOn = false;
  void applyColor();
};
