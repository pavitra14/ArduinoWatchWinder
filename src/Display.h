#pragma once
#include <Arduino.h>

class Display {
public:
  bool begin();
  void showDigit(uint8_t digit);
  void showMemory();
  bool available() const { return ready; }
private:
  bool ready = false;
};
