#include "Display.h"

#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_UNOR4_MINIMA)
#define HAS_LED_MATRIX 1
#include "Arduino_LED_Matrix.h"
static ArduinoLEDMatrix matrix;
#else
#define HAS_LED_MATRIX 0
#endif

#if HAS_LED_MATRIX
void Display::renderFrame(uint8_t frame[8][12]) {
  if (!ready) return;
  if (orientation == MatrixOrientation::Normal) {
    matrix.renderBitmap(frame, 8, 12);
    return;
  }
  uint8_t flipped[8][12] = {};
  for (uint8_t r = 0; r < 8; r++) {
    for (uint8_t c = 0; c < 12; c++) {
      flipped[7 - r][11 - c] = frame[r][c]; // rotate 180 for upside-down mount
    }
  }
  matrix.renderBitmap(flipped, 8, 12);
}
#endif

bool Display::begin() {
#if HAS_LED_MATRIX
  ready = matrix.begin();
#else
  ready = false;
#endif
  return ready;
}

#if HAS_LED_MATRIX
void renderDigit(Display &disp, uint8_t digit) {
  static const uint8_t DIGIT_FONT[10][5] = {
    {0b111,0b101,0b101,0b101,0b111}, //0
    {0b010,0b110,0b010,0b010,0b111}, //1
    {0b111,0b001,0b111,0b100,0b111}, //2
    {0b111,0b001,0b111,0b001,0b111}, //3
    {0b101,0b101,0b111,0b001,0b001}, //4
    {0b111,0b100,0b111,0b001,0b111}, //5
    {0b111,0b100,0b111,0b101,0b111}, //6
    {0b111,0b001,0b011,0b001,0b001}, //7
    {0b111,0b101,0b111,0b101,0b111}, //8
    {0b111,0b101,0b111,0b001,0b111}  //9
  };
  uint8_t frame[8][12] = {};
  digit %= 10;
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      if (DIGIT_FONT[digit][row] & (1 << (2 - col))) {
        frame[row + 1][col + 4] = 1;
      }
    }
  }
  disp.renderFrame(frame);
}
#endif

void Display::showDigit(uint8_t digit) {
#if HAS_LED_MATRIX
  if (!ready) return;
  renderDigit(*this, digit);
#else
  (void)digit;
#endif
}

void Display::showMemory() {
#if HAS_LED_MATRIX
  if (!ready) return;
  const uint8_t letterM[5] = {
    0b10101,
    0b11111,
    0b10101,
    0b10101,
    0b10101
  };
  uint8_t frame[8][12] = {};
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 5; col++) {
      if (letterM[row] & (1 << (4 - col))) {
        frame[row + 1][col + 3] = 1;
      }
    }
  }
  renderFrame(frame);
#endif
}
