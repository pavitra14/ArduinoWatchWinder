#pragma once
#include <Arduino.h>
#include <IRRemoteMap.h>
#include "StepperController.h"
#include "RGBController.h"

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

enum class MotorSelection { MOTOR1, MOTOR2, BOTH };
enum class PresetMode { CONTINUOUS, DUTY_CYCLE };

struct PresetConfig {
  uint8_t id;                // human readable id (matches remote number where applicable)
  IRButton button;           // remote button that selects this preset
  const char *label;         // short label for debug
  MotorSelection motors;     // which motors to run
  StepperDir direction;      // CW/CCW for motor1 (or both when matching)
  StepperDir motor2Direction;// optional override for motor2 (defaults to direction when matching)
  bool alternateDirection;   // when true, duty cycle flips direction each run window
  StepperDir altDirection;   // alternate direction used when flipping
  PresetMode mode;           // continuous or duty-cycle
  uint32_t runMs;            // used only for duty cycle
  uint32_t restMs;           // used only for duty cycle
  RgbColor color;            // LED color
  uint8_t brightness;        // LED brightness 0..255
  StepperSpeed speed;        // stepper speed for this preset
};

const PresetConfig* findPreset(IRButton button);
const PresetConfig* findPresetById(uint8_t id);

class DualStepperManager {
public:
  DualStepperManager(StepperController &m1, StepperController &m2);
  void begin(uint32_t stepsPerRevolution);
  void start(MotorSelection target,
             StepperDir dirMotor1,
             StepperSpeed speed,
             StepperDir dirMotor2);
  void stopAll();
  void stepTick();
  void testSequential(uint32_t stepsPerRev);
private:
  StepperController &motor1;
  StepperController &motor2;
};

enum class RunnerPhase { Idle, Running, Resting };

class PresetRunner {
public:
  bool start(const PresetConfig* preset,
             DualStepperManager &steppers,
             RGBController &rgb,
             unsigned long nowMillis);
  void stop(DualStepperManager &steppers, RGBController &rgb);
  void tick(DualStepperManager &steppers, RGBController &rgb, unsigned long nowMillis);
  bool isRunning() const { return phase == RunnerPhase::Running; }
  bool isActive() const { return phase != RunnerPhase::Idle; }
  const PresetConfig* active() const { return activePreset; }
  RunnerPhase phaseState() const { return phase; }
private:
  void resolveNextRunDirections(StepperDir &dir1Out, StepperDir &dir2Out) const;

  const PresetConfig* activePreset = nullptr;
  RunnerPhase phase = RunnerPhase::Idle;
  unsigned long phaseStart = 0;
  unsigned long startedAt = 0;
  StepperDir currentDir = StepperDir::CW;
  StepperDir currentDirMotor2 = StepperDir::CW;
};

class PresetStore {
public:
  bool save(uint8_t presetId);
  bool load(uint8_t &presetIdOut);
};
