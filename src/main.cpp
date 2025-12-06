#include <Arduino.h>
#include <IRremote.hpp>
#include "Arduino_LED_Matrix.h"
#include "IRRemoteMap.h"
#include "PresetSystem.h"
#include "StepperController.h"
#include "RGBController.h"

// Pin mapping
constexpr uint8_t IR_PIN = 2;
constexpr uint8_t R_PIN = 6;  // PWM
constexpr uint8_t G_PIN = 5;  // PWM
constexpr uint8_t B_PIN = 3;  // PWM

// Stepper 1 (existing)
constexpr uint8_t STP1_IN1 = 7;
constexpr uint8_t STP1_IN2 = 8;
constexpr uint8_t STP1_IN3 = 18; // SDA
constexpr uint8_t STP1_IN4 = 19; // SCL
// Stepper 2 (new)
constexpr uint8_t STP2_IN1 = 4;
constexpr uint8_t STP2_IN2 = 9;
constexpr uint8_t STP2_IN3 = 10;
constexpr uint8_t STP2_IN4 = 11;

constexpr uint32_t STEPS_PER_REV = 4096UL;

ArduinoLEDMatrix matrix;
RGBController rgb(R_PIN, G_PIN, B_PIN);
StepperController stepper1(STP1_IN1, STP1_IN2, STP1_IN3, STP1_IN4);
StepperController stepper2(STP2_IN1, STP2_IN2, STP2_IN3, STP2_IN4);
DualStepperManager steppers(stepper1, stepper2);
PresetRunner presetRunner;
PresetStore presetStore;

const PresetConfig* selectedPreset = nullptr;
uint8_t lastRunningPresetId = 0;
bool needPresetBlink = false;
bool blinkState = false;
unsigned long lastBlinkToggle = 0;

void renderDigit(uint8_t digit) {
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
  matrix.renderBitmap(frame, 8, 12);
}

void showPresetOnMatrix(uint8_t presetId) {
  renderDigit(presetId % 10);
}

void showMemoryMessage() {
  // Render a simple "M" to indicate memory load
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
  matrix.renderBitmap(frame, 8, 12);
}

void logStatus(const char* msg) {
  Serial.print(F("[Status] "));
  Serial.println(msg);
}

void syncLedToState() {
  if (presetRunner.isActive() && presetRunner.active()) {
    const PresetConfig* p = presetRunner.active();
    rgb.setBrightness(p->brightness);
    rgb.setColor(p->color.r, p->color.g, p->color.b);
    rgb.setOn(presetRunner.phaseState() == RunnerPhase::Running);
  } else if (selectedPreset) {
    rgb.setBrightness(selectedPreset->brightness);
    rgb.setColor(selectedPreset->color.r, selectedPreset->color.g, selectedPreset->color.b);
    rgb.setOn(true);
  } else {
    rgb.setColor(255, 0, 0); // default stopped
    rgb.setOn(true);
  }
}

void flashBlueRed(uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    rgb.setBrightness(255);
    rgb.setColor(0, 0, 255);
    rgb.setOn(true);
    delay(120);
    rgb.setColor(255, 0, 0);
    rgb.setOn(true);
    delay(120);
  }
  syncLedToState();
}

void selectPreset(const PresetConfig* preset) {
  if (!preset) return;
  if (presetRunner.isActive()) {
    presetRunner.stop(steppers, rgb);
  }
  selectedPreset = preset;
  needPresetBlink = false;
  syncLedToState();
  showPresetOnMatrix(preset->id);
  Serial.print(F("[Preset] Selected id="));
  Serial.print(preset->id);
  Serial.print(F(" label="));
  Serial.println(preset->label);
}

void startSelectedPreset(unsigned long now) {
  if (!selectedPreset) {
    needPresetBlink = true;
    logStatus("No preset selected; need selection.");
    return;
  }
  if (presetRunner.start(selectedPreset, steppers, rgb, now)) {
    lastRunningPresetId = selectedPreset->id;
    showPresetOnMatrix(selectedPreset->id);
    Serial.print(F("[Preset] Started id="));
    Serial.println(selectedPreset->id);
  }
}

void stopPreset() {
  if (!presetRunner.isActive()) return;
  presetRunner.stop(steppers, rgb);
  logStatus("Preset stopped");
}

void resetBoard() {
#ifdef __arm__
  NVIC_SystemReset();
#else
  void(*resetFunc)(void) = 0;
  resetFunc();
#endif
  while (true) { delay(10); }
}

void runSystemCheck() {
  stopPreset();
  logStatus("System check: LED sweep + motor test");
  rgb.setBrightness(255);
  const RgbColor colors[] = {
    {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
    {255, 255, 0}, {0, 255, 255}, {255, 0, 255}
  };
  for (int i = 0; i < 6; i++) {
    rgb.setColor(colors[i].r, colors[i].g, colors[i].b);
    rgb.setOn(true);
    delay(120);
  }
  steppers.testSequential(STEPS_PER_REV);
  syncLedToState();
}

void handleSavePreset() {
  if (lastRunningPresetId == 0) {
    Serial.println(F("[Preset] Save skipped (no last running preset)"));
    flashBlueRed(1);
    return;
  }
  bool ok = presetStore.save(lastRunningPresetId);
  Serial.print(F("[Preset] Save request id="));
  Serial.print(lastRunningPresetId);
  Serial.println(ok ? F(" OK") : F(" FAIL"));
  flashBlueRed(ok ? 3 : 1);
}

void handleLoadPreset() {
  uint8_t id = 0;
  if (presetStore.load(id)) {
    const PresetConfig* preset = findPresetById(id);
    if (preset) {
      selectPreset(preset);
      Serial.print(F("[Preset] Loaded id="));
      Serial.println(id);
      flashBlueRed(3);
      return;
    }
  }
  Serial.println(F("[Preset] Load failed"));
  flashBlueRed(1);
}

void handleIRButton(IRButton btn, unsigned long now) {
  const PresetConfig* preset = findPreset(btn);
  if (preset) {
    selectPreset(preset);
    return;
  }

  switch (btn) {
    case IRButton::BTN_OK:
      if (presetRunner.isActive()) stopPreset();
      else startSelectedPreset(now);
      break;
    case IRButton::BTN_9:
      handleSavePreset();
      break;
    case IRButton::BTN_0:
      handleLoadPreset();
      break;
    case IRButton::BTN_HASH:
      runSystemCheck();
      break;
    case IRButton::BTN_STAR:
      logStatus("Reset requested");
      resetBoard();
      break;
    case IRButton::BTN_UP:
    case IRButton::BTN_DOWN:
    case IRButton::BTN_LEFT:
    case IRButton::BTN_RIGHT:
      logStatus("Reserved button pressed");
      break;
    default:
      break;
  }
}

void processIr(unsigned long now) {
  if (!IrReceiver.decode()) return;
  const uint32_t raw = IrReceiver.decodedIRData.decodedRawData;
  const IRButton btn = IRRemoteMap::getButton(raw);
  if (btn == IRButton::BTN_UNKNOWN) {
    IrReceiver.resume();
    return; // keep logs clean for unknown/unmapped keys
  }
  Serial.print(F("[IR] Button="));
  Serial.println(IRRemoteMap::toString(btn));
  handleIRButton(btn, now);
  IrReceiver.resume();
}

void handlePresetBlink(unsigned long now) {
  if (!needPresetBlink) return;
  if (selectedPreset) {
    needPresetBlink = false;
    return;
  }
  if (now - lastBlinkToggle >= 250) {
    lastBlinkToggle = now;
    blinkState = !blinkState;
    rgb.setColor(blinkState ? 255 : 0, 0, blinkState ? 0 : 255);
    rgb.setOn(true);
  }
}

void setup() {
  Serial.begin(9600);
  delay(50);
  Serial.println(F("\n[Boot] Watch Winder Starting..."));

  if (matrix.begin()) {
    Serial.println(F("[Boot] Matrix init OK"));
  } else {
    Serial.println(F("[Boot] Matrix init failed"));
  }

  rgb.begin();
  rgb.setColor(255, 255, 0);
  rgb.setOn(true);
  Serial.println(F("[Boot] RGB init complete (yellow warmup)"));

  // Attempt to load last preset from memory
  uint8_t bootPresetId = 0;
  if (presetStore.load(bootPresetId)) {
    const PresetConfig* bootPreset = findPresetById(bootPresetId);
    if (bootPreset) {
      Serial.print(F("[Boot] Loaded preset from memory id="));
      Serial.println(bootPresetId);
      rgb.setBrightness(255);
      rgb.setColor(255, 64, 128); // pink flash to indicate memory load
      rgb.setOn(true);
      showMemoryMessage();
      delay(400);
      selectPreset(bootPreset);
    } else {
      Serial.println(F("[Boot] Memory preset id invalid"));
    }
  } else {
    Serial.println(F("[Boot] No valid preset in memory"));
  }

  steppers.begin(STEPS_PER_REV);
  Serial.println(F("[Boot] Steppers initialized"));

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("[Boot] IR ready"));

  stepper1.testMotor();
  Serial.println(F("[Boot] Motor1 test done"));
  stepper2.testMotor();
  Serial.println(F("[Boot] Motor2 test done"));

  rgb.setColor(255, 0, 0);
  rgb.setOn(true);
  logStatus("Ready");
}

void loop() {
  const unsigned long now = millis();
  processIr(now);
  steppers.stepTick();
  presetRunner.tick(steppers, rgb, now);
  handlePresetBlink(now);
  delay(1);
}
