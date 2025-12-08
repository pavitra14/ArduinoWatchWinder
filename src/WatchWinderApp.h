#pragma once
#include <Arduino.h>
#include "PresetSystem.h"
#include "StepperController.h"
#include "RGBController.h"
#include "Display.h"

enum class IRButton : uint8_t;

class WatchWinderApp {
public:
  void begin();
  void tick();

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
  static constexpr uint8_t STP2_IN1 = 4;
  static constexpr uint8_t STP2_IN2 = 9;
  static constexpr uint8_t STP2_IN3 = 10;
  static constexpr uint8_t STP2_IN4 = 11;
  static constexpr uint32_t STEPS_PER_REV = 4096UL;
  static constexpr MatrixOrientation MATRIX_ORIENTATION = MatrixOrientation::UpsideDown; // set to UpsideDown if board is flipped

  Display display;
  RGBController rgb{R_PIN, G_PIN, B_PIN};
  StepperController stepper1{STP1_IN1, STP1_IN2, STP1_IN3, STP1_IN4};
  StepperController stepper2{STP2_IN1, STP2_IN2, STP2_IN3, STP2_IN4};
  DualStepperManager steppers{stepper1, stepper2};
  PresetRunner presetRunner;
  PresetStore presetStore;

  const PresetConfig* selectedPreset = nullptr;
  uint8_t lastRunningPresetId = 0;
  bool needPresetBlink = false;
  bool blinkState = false;
  unsigned long lastBlinkToggle = 0;

  void logStatus(const char* msg);
  void syncLedToState();
  void flashBlueRed(uint8_t times);
  void selectPreset(const PresetConfig* preset);
  void startSelectedPreset(unsigned long now);
  void stopPreset();
  void resetBoard();
  void runSystemCheck();
  void handleSavePreset();
  void handleLoadPreset();
  void handleIRButton(IRButton btn, unsigned long now);
  void handleIr(unsigned long now);
  void handlePresetBlink(unsigned long now);
};
