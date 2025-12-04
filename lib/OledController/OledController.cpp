#include "OledController.h"

OledController::OledController(uint8_t address)
  : _address(address), _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
}

bool OledController::begin() {
  Serial.print("OLED Configured (SH1106): ");
  Serial.print(SCREEN_WIDTH);
  Serial.print("x");
  Serial.println(SCREEN_HEIGHT);

  // SH1106 initialization
  if(!_display.begin(_address, true)) {
    return false;
  }
  _display.clearDisplay();
  _display.setTextColor(SH110X_WHITE);
  _display.display();
  return true;
}

void OledController::clear() {
  _display.clearDisplay();
  _display.display();
}

void OledController::updateDashboard(const char *statusText, const char *directionText, int32_t rotationsTenth) {
  // Multi-pass hard clear: black -> white -> black to fully reset panel then draw.
  _display.clearDisplay();
  _display.fillScreen(SH110X_BLACK);
  _display.display();
  delay(1000);

  _display.clearDisplay();
  _display.fillScreen(SH110X_WHITE);
  _display.display();
  delay(1000);

  _display.clearDisplay();
  _display.fillScreen(SH110X_BLACK);
  _display.display();
  _display.clearDisplay(); // ensure local buffer is empty

  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE, SH110X_BLACK);
  _display.setTextWrap(false);

  // Line 1: Status
  _display.setCursor(0, 2);
  _display.print(F("Status: "));
  _display.println(statusText);

  // Line 2: Direction
  _display.setCursor(0, 16);
  _display.print(F("Direction: "));
  _display.println(directionText);

  // Line 3: Rotations
  const int32_t whole = rotationsTenth / 10;
  const uint8_t frac = (uint8_t)abs(rotationsTenth % 10);
  char rotBuf[20];
  snprintf(rotBuf, sizeof(rotBuf), "Rotations: %ld.%01u", (long)whole, frac);
  _display.setCursor(0, 30);
  _display.print(rotBuf);

  // Bottom hint
  _display.setCursor(0, 48);
  _display.print(F("Press OK to update"));

  _display.display();
}

void OledController::showMessage(const char *msg) {
  _display.clearDisplay();
  _display.setTextSize(2);
  _display.setCursor(0, 0);
  _display.println(msg);
  _display.display();
}
