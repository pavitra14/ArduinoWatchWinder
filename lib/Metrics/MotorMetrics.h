#pragma once
#include <Arduino.h>

enum class StepperDir;

enum class MotorId : uint8_t { Motor1 = 0, Motor2 = 1 };

struct MotorSnapshot {
  uint32_t cwSteps = 0;
  uint32_t ccwSteps = 0;
  float turnsCW = 0;
  float turnsCCW = 0;
  float turnsPerMinute = 0;
  float turnsPerDay = 0;
  uint32_t startCount = 0;
  uint32_t runSeconds = 0;
};

class MotorMetrics {
public:
  void begin(uint32_t stepsPerRev);
  void recordStep(MotorId id, StepperDir dir);
  void recordStart(MotorId id);
  void recordStop(MotorId id);
  MotorSnapshot snapshot(MotorId id);
  uint32_t stepsPerRevolution() const { return stepsPerRev; }

private:
  struct Counters {
    uint32_t cwSteps = 0;
    uint32_t ccwSteps = 0;
    uint32_t minuteSteps = 0;
    unsigned long minuteWindowStart = 0;
    uint32_t daySteps = 0;
    unsigned long dayWindowStart = 0;
    uint32_t startCount = 0;
    uint32_t runMs = 0;
    unsigned long lastStepTs = 0;
    bool running = false;
  };

  Counters counters[2];
  uint32_t stepsPerRev = 4096;

  void refreshWindows(Counters &c, unsigned long now) const;
  static uint8_t idxFor(MotorId id) { return id == MotorId::Motor1 ? 0 : 1; }
};
