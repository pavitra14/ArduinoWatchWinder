#pragma once
#include <Arduino.h>

enum class MatrixOrientation { Normal, UpsideDown };

class Display {
public:
  bool begin();
  void setOrientation(MatrixOrientation o) { orientation = o; }
  void showDigit(uint8_t digit);
  void showMemory();
  bool available() const { return ready; }
private:
  // Allow helper renderer in implementation to call renderFrame.
  friend void renderDigit(Display &disp, uint8_t digit);

  bool ready = false;
  MatrixOrientation orientation = MatrixOrientation::Normal;
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
  void renderFrame(uint8_t frame[8][12]);
#endif
};
