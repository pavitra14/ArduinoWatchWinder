#pragma once
#include <Arduino.h>

enum class StepperSpeed { SLOW, NORMAL, FAST };
enum class StepperDir { CW, CCW };

class StepperController {
public:
  StepperController(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4);
  void begin();
  void setStepsPerRevolution(uint32_t sprev);

  // blocking rotate
  void rotate(StepperDir dir, uint32_t steps, StepperSpeed speed);
  void rotateRevolutions(StepperDir dir, float revolutions, StepperSpeed speed);

  // non-blocking continuous control
  void startContinuous(StepperDir dir, StepperSpeed speed);
  void stopContinuous();
  void stepTick(); // must be called frequently from loop()

  // position tracking & absolute move
  int32_t getPosition() const; // current position in half-steps (signed)
  void moveTo(int32_t targetSteps, StepperSpeed speed); // blocking absolute move (uses rotate)

  // quick test
  void testMotor();

private:
  uint8_t pins[4];
  uint32_t stepsPerRevolution = 4096;
  uint8_t seqIndex = 0;

  // continuous-run vars
  bool continuousRunning = false;
  StepperDir continuousDir = StepperDir::CW;
  StepperSpeed continuousSpeed = StepperSpeed::NORMAL;
  unsigned long lastStepMillis = 0;
  uint16_t currentStepIntervalMs = 4; // ms between half-steps

  volatile int32_t positionSteps = 0; // signed half-step count (can overflow if huge; handle if needed)

  void stepOnce(uint8_t stepIndex);
  void setIntervalForSpeed(StepperSpeed speed);
};
