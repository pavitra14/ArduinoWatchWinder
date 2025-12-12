#include "MotorMetrics.h"
#include "StepperController.h"

void MotorMetrics::begin(uint32_t stepsPerRevIn) {
  stepsPerRev = stepsPerRevIn > 0 ? stepsPerRevIn : 4096;
  unsigned long now = millis();
  for (auto &c : counters) {
    c = Counters{};
    c.minuteWindowStart = now;
    c.dayWindowStart = now;
    c.lastStepTs = 0;
    c.running = false;
  }
}

void MotorMetrics::refreshWindows(Counters &c, unsigned long now) const {
  static const unsigned long MINUTE_MS = 60000UL;
  static const unsigned long DAY_MS = 86400000UL;

  if (now - c.minuteWindowStart >= MINUTE_MS) {
    c.minuteSteps = 0;
    c.minuteWindowStart = now;
  }
  if (now - c.dayWindowStart >= DAY_MS) {
    c.daySteps = 0;
    c.dayWindowStart = now;
  }
}

void MotorMetrics::recordStart(MotorId id) {
  Counters &c = counters[idxFor(id)];
  c.startCount++;
  c.running = true;
}

void MotorMetrics::recordStop(MotorId id) {
  Counters &c = counters[idxFor(id)];
  c.running = false;
  c.lastStepTs = 0;
}

void MotorMetrics::recordStep(MotorId id, StepperDir dir) {
  uint8_t idx = idxFor(id);
  Counters &c = counters[idx];
  unsigned long now = millis();
  refreshWindows(c, now);
  if (c.running && c.lastStepTs != 0 && now >= c.lastStepTs) {
    c.runMs += (uint32_t)(now - c.lastStepTs);
  }
  c.lastStepTs = now;
  if (dir == StepperDir::CW) {
    c.cwSteps++;
  } else {
    c.ccwSteps++;
  }
  c.minuteSteps++;
  c.daySteps++;
}

MotorSnapshot MotorMetrics::snapshot(MotorId id) {
  MotorSnapshot snap;
  Counters &c = counters[idxFor(id)];
  refreshWindows(c, millis());
  snap.cwSteps = c.cwSteps;
  snap.ccwSteps = c.ccwSteps;
  snap.turnsCW = (float)c.cwSteps / (float)stepsPerRev;
  snap.turnsCCW = (float)c.ccwSteps / (float)stepsPerRev;
  snap.turnsPerMinute = (float)c.minuteSteps / (float)stepsPerRev;
  snap.turnsPerDay = (float)c.daySteps / (float)stepsPerRev;
  snap.startCount = c.startCount;
  snap.runSeconds = c.runMs / 1000UL;
  return snap;
}
