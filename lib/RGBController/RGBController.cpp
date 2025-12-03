#include "RGBController.h"
RGBController::RGBController(uint8_t rPin, uint8_t gPin, uint8_t bPin)
: rPin(rPin), gPin(gPin), bPin(bPin), curR(0), curG(0), curB(0) {}

void RGBController::begin(){
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
  analogWrite(rPin,0); analogWrite(gPin,0); analogWrite(bPin,0);
}

void RGBController::setBrightness(uint8_t b){ brightness = b; if(isOn) applyColor(); }

void RGBController::applyColor(){
  uint16_t r32 = (uint16_t)curR * (uint16_t)brightness;
  uint16_t g32 = (uint16_t)curG * (uint16_t)brightness;
  uint16_t b32 = (uint16_t)curB * (uint16_t)brightness;
  analogWrite(rPin, (uint8_t)(r32 >> 8));
  analogWrite(gPin, (uint8_t)(g32 >> 8));
  analogWrite(bPin, (uint8_t)(b32 >> 8));
}

void RGBController::setColor(uint8_t r, uint8_t g, uint8_t b){
  curR = r; curG = g; curB = b;
  if(isOn) applyColor();
}

void RGBController::setOn(bool on){
  isOn = on;
  if(isOn) applyColor();
  else { analogWrite(rPin,0); analogWrite(gPin,0); analogWrite(bPin,0); }
}

void RGBController::setRed(){ setColor(255,0,0); setOn(true); }
void RGBController::setGreen(){ setColor(0,255,0); setOn(true); }
void RGBController::setOff(){ setOn(false); }
