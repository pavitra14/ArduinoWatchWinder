#include "StepperController.h"

// Half-step sequence (8-step)
static const uint8_t HALFSTEP_SEQ[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

StepperController::StepperController(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
  pins[0] = in1; pins[1] = in2; pins[2] = in3; pins[3] = in4;
}

void StepperController::begin() {
  for (int i=0;i<4;i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }
  seqIndex = 0;
  continuousRunning = false;
  lastStepMillis = 0;
  setIntervalForSpeed(continuousSpeed);
  positionSteps = 0;
}

void StepperController::setStepsPerRevolution(uint32_t sprev){ stepsPerRevolution = sprev; }

void StepperController::stepOnce(uint8_t stepIndex){
  for (int i=0;i<4;i++) {
    digitalWrite(pins[i], HALFSTEP_SEQ[stepIndex & 0x07][i] ? HIGH : LOW);
  }
}

void StepperController::setIntervalForSpeed(StepperSpeed speed){
  switch (speed) {
    case StepperSpeed::SLOW:   currentStepIntervalMs = 8;  break;
    case StepperSpeed::NORMAL: currentStepIntervalMs = 4;  break;
    case StepperSpeed::FAST:   currentStepIntervalMs = 2;  break;
    default: currentStepIntervalMs = 4;
  }
}

/* Blocking rotate (keeps old interface) */
void StepperController::rotate(StepperDir dir, uint32_t steps, StepperSpeed speed){
  uint16_t oldInterval = currentStepIntervalMs;
  setIntervalForSpeed(speed);

  for (uint32_t s=0; s<steps; ++s){
    if (dir == StepperDir::CW) {
      seqIndex = (seqIndex + 1) & 0x07;
      positionSteps++; // increment position for each half-step (CW positive)
    } else {
      seqIndex = (seqIndex + 7) & 0x07;
      positionSteps--; // CCW negative
    }
    stepOnce(seqIndex);
    delay(currentStepIntervalMs);
  }
  for (int i=0;i<4;i++) digitalWrite(pins[i], LOW);
  currentStepIntervalMs = oldInterval;
}

void StepperController::rotateRevolutions(StepperDir dir, float revolutions, StepperSpeed speed){
  uint32_t steps = (uint32_t)(revolutions * (float)stepsPerRevolution);
  rotate(dir, steps, speed);
}

/* Non-blocking continuous run */
void StepperController::startContinuous(StepperDir dir, StepperSpeed speed){
  continuousDir = dir;
  continuousSpeed = speed;
  setIntervalForSpeed(speed);
  continuousRunning = true;
  lastStepMillis = millis();
}

void StepperController::stopContinuous(){
  continuousRunning = false;
  for (int i=0;i<4;i++) digitalWrite(pins[i], LOW);
}

/* Call this from loop() frequently */
void StepperController::stepTick(){
  if (!continuousRunning) return;
  unsigned long now = millis();
  if (now - lastStepMillis >= currentStepIntervalMs) {
    lastStepMillis = now;
    if (continuousDir == StepperDir::CW) {
      seqIndex = (seqIndex + 1) & 0x07;
      positionSteps++; // update position
    } else {
      seqIndex = (seqIndex + 7) & 0x07;
      positionSteps--; // update position
    }
    stepOnce(seqIndex);
  }
}

void StepperController::testMotor(){
  // small blocking sweep
  rotate(StepperDir::CW, 50, StepperSpeed::FAST);
  delay(80);
  rotate(StepperDir::CCW, 50, StepperSpeed::FAST);
}

/* position accessor and absolute move */
int32_t StepperController::getPosition() const {
  return positionSteps;
}

void StepperController::moveTo(int32_t targetSteps, StepperSpeed speed) {
  if (targetSteps == positionSteps) return;
  if (targetSteps > positionSteps) {
    uint32_t delta = (uint32_t)(targetSteps - positionSteps);
    rotate(StepperDir::CW, delta, speed);
  } else {
    uint32_t delta = (uint32_t)(positionSteps - targetSteps);
    rotate(StepperDir::CCW, delta, speed);
  }
}
