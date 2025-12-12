#pragma once
#include <Arduino.h>
#include "PresetSystem.h"
#include "StepperController.h"
#include "RGBController.h"
#include "Display.h"
#include "WifiModule.h"
#include "MotorMetrics.h"

enum class IRButton : uint8_t;

class WatchWinderApp {
public:
  void begin();
  void tick();

  // WiFi control surface (optional)
  bool wifiStartPreset(uint8_t presetId, unsigned long now);
  void wifiStopPreset();
  const PresetConfig* wifiSelectedPreset() const { return selectedPreset; }
  const PresetConfig* wifiActivePreset() const { return presetRunner.active(); }
  bool wifiIsRunning() const { return presetRunner.isRunning(); }
  uint8_t wifiLastRunPresetId() const { return lastRunningPresetId; }
  MotorSnapshot wifiMotorSnapshot(MotorId id);
  bool wifiSchedulePreset(uint8_t presetId, uint32_t delaySeconds);
  bool wifiCancelSchedule();
  bool wifiScheduleActive() const { return scheduleArmed; }
  uint32_t wifiScheduleSecondsRemaining(unsigned long now) const;
  uint8_t wifiSchedulePresetId() const { return scheduleArmed ? schedulePresetId : 0; }
  bool wifiManualStart(MotorSelection motors, StepperDir dir1, StepperDir dir2, StepperSpeed speed);
  void wifiManualStop();
  bool wifiManualIsActive() const { return manualRunning; }
  MotorSelection wifiManualMotors() const { return manualMotors; }
  StepperDir wifiManualDir1() const { return manualDir1; }
  StepperDir wifiManualDir2() const { return manualDir2; }
  StepperSpeed wifiManualSpeed() const { return manualSpeed; }
  uint32_t wifiScheduleRunCount() const { return scheduleRunCount; }
  bool wifiAllowed() const { return wifiEnabledPref; }

private:
  // Pin mapping
  static constexpr uint8_t IR_PIN = 2;
  static constexpr uint8_t R_PIN = 6;  // PWM
  static constexpr uint8_t G_PIN = 5;  // PWM
  static constexpr uint8_t B_PIN = 3;  // PWM
  static constexpr uint8_t STP1_IN1 = 7;
  static constexpr uint8_t STP1_IN2 = 8;
  static constexpr uint8_t STP1_IN3 = 18; // SDA
  static constexpr uint8_t STP1_IN4 = 19; // SCL
  // Avoid SPI pins 10/11 used by WiFi; place motor2 on spare analog-capable pins.
  static constexpr uint8_t STP2_IN1 = 14; // A0
  static constexpr uint8_t STP2_IN2 = 15; // A1
  static constexpr uint8_t STP2_IN3 = 16; // A2
  static constexpr uint8_t STP2_IN4 = 17; // A3
  static constexpr uint32_t STEPS_PER_REV = 4096UL;
  static constexpr MatrixOrientation MATRIX_ORIENTATION = MatrixOrientation::UpsideDown; // set to UpsideDown if board is flipped
  static constexpr unsigned long WIFI_MAX_ON_MS = 300000UL; // 5 minutes

  Display display;
  RGBController rgb{R_PIN, G_PIN, B_PIN};
  StepperController stepper1{STP1_IN1, STP1_IN2, STP1_IN3, STP1_IN4};
  StepperController stepper2{STP2_IN1, STP2_IN2, STP2_IN3, STP2_IN4};
  DualStepperManager steppers{stepper1, stepper2};
  PresetRunner presetRunner;
  PresetStore presetStore;
  WifiModule wifi;
  MotorMetrics metrics;

  const PresetConfig* selectedPreset = nullptr;
  uint8_t lastRunningPresetId = 0;
  bool needPresetBlink = false;
  bool blinkState = false;
  unsigned long lastBlinkToggle = 0;
  bool scheduleArmed = false;
  uint8_t schedulePresetId = 0;
  unsigned long scheduleDelayMs = 0;
  unsigned long scheduleArmedAt = 0;
  uint32_t scheduleRunCount = 0;
  bool manualRunning = false;
  MotorSelection manualMotors = MotorSelection::BOTH;
  StepperDir manualDir1 = StepperDir::CW;
  StepperDir manualDir2 = StepperDir::CW;
  StepperSpeed manualSpeed = StepperSpeed::NORMAL;
  bool wifiEnabledPref = false;
  unsigned long wifiEnabledAt = 0;

  void logStatus(const char* msg);
  void syncLedToState();
  void flashBlueRed(uint8_t times);
  void selectPreset(const PresetConfig* preset);
  void startSelectedPreset(unsigned long now);
  void stopPreset();
  void parkMotors();
  void resetBoard();
  void runSystemCheck();
  void handleSavePreset();
  void handleLoadPreset();
  void handleIRButton(IRButton btn, unsigned long now);
  void handleIr(unsigned long now);
  void handlePresetBlink(unsigned long now);
};
