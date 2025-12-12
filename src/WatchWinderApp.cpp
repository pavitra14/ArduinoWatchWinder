#include <IRremote.hpp>
#include "IRRemoteMap.h"
#include "WatchWinderApp.h"

void WatchWinderApp::logStatus(const char* msg) {
  Serial.print(F("[Status] "));
  Serial.println(msg);
}

void WatchWinderApp::syncLedToState() {
  if (manualRunning) {
    rgb.setBrightness(255);
    rgb.setColor(0, 255, 180); // cyan-green for manual mode
    rgb.setOn(true);
  } else if (presetRunner.isActive() && presetRunner.active()) {
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

void WatchWinderApp::flashBlueRed(uint8_t times) {
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

void WatchWinderApp::selectPreset(const PresetConfig* preset) {
  if (!preset) return;
  if (presetRunner.isActive()) {
    presetRunner.stop(steppers, rgb);
  }
  if (manualRunning) {
    steppers.stopAll();
    manualRunning = false;
  }
  selectedPreset = preset;
  needPresetBlink = false;
  syncLedToState();
  display.showDigit(preset->id);
  Serial.print(F("[Preset] Selected id="));
  Serial.print(preset->id);
  Serial.print(F(" label="));
  Serial.println(preset->label);
}

void WatchWinderApp::startSelectedPreset(unsigned long now) {
  if (!selectedPreset) {
    needPresetBlink = true;
    logStatus("No preset selected; need selection.");
    return;
  }
  if (presetRunner.start(selectedPreset, steppers, rgb, now)) {
    lastRunningPresetId = selectedPreset->id;
    display.showDigit(selectedPreset->id);
    Serial.print(F("[Preset] Started id="));
    Serial.println(selectedPreset->id);
  }
}

void WatchWinderApp::stopPreset() {
  if (presetRunner.isActive()) {
    presetRunner.stop(steppers, rgb);
    logStatus("Preset stopped");
  }
  if (manualRunning) {
    steppers.stopAll();
    manualRunning = false;
    syncLedToState();
    logStatus("Manual control stopped");
  }
}

void WatchWinderApp::parkMotors() {
  // stop any active motion then return both motors to logical zero
  stopPreset();
  steppers.stopAll();
  logStatus("Parking motors to zero");
  stepper1.moveTo(0, StepperSpeed::FAST);
  stepper2.moveTo(0, StepperSpeed::FAST);
}

void WatchWinderApp::resetBoard() {
#ifdef __arm__
  NVIC_SystemReset();
#else
  void(*resetFunc)(void) = 0;
  resetFunc();
#endif
  while (true) { delay(10); }
}

void WatchWinderApp::runSystemCheck() {
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

void WatchWinderApp::handleSavePreset() {
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

void WatchWinderApp::handleLoadPreset() {
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

void WatchWinderApp::handleIRButton(IRButton btn, unsigned long now) {
  const PresetConfig* preset = findPreset(btn);
  if (preset) {
    selectPreset(preset);
    return;
  }

  switch (btn) {
    case IRButton::BTN_OK:
      if (presetRunner.isActive() || manualRunning) stopPreset();
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
      logStatus("Reset requested; parking motors");
      parkMotors();
      resetBoard();
      break;
    case IRButton::BTN_UP:
      wifiEnabledPref = true;
      presetStore.saveWifiEnabled(true);
      if (!wifi.isEnabled()) {
        wifi.begin(this);
        logStatus("WiFi enabled");
        wifiEnabledAt = millis();
      } else {
        logStatus("WiFi already enabled");
      }
      break;
    case IRButton::BTN_DOWN:
      wifiEnabledPref = false;
      presetStore.saveWifiEnabled(false);
      if (wifi.isEnabled()) {
        wifi.disable();
        logStatus("WiFi disabled");
      }
      wifiEnabledAt = 0;
      break;
    case IRButton::BTN_LEFT:
    case IRButton::BTN_RIGHT:
      logStatus("Reserved button pressed");
      break;
    default:
      break;
  }
}

void WatchWinderApp::handleIr(unsigned long now) {
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

void WatchWinderApp::handlePresetBlink(unsigned long now) {
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

void WatchWinderApp::begin() {
  Serial.begin(9600);
  delay(50);
  Serial.println(F("\n[Boot] Watch Winder Starting..."));

  bool matrixOk = display.begin();
  display.setOrientation(MATRIX_ORIENTATION);
  if (matrixOk) {
    Serial.println(F("[Boot] Matrix init OK"));
  } else {
    Serial.println(F("[Boot] Matrix not available on this board (skipping)"));
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
      display.showMemory();
      delay(400);
      selectPreset(bootPreset);
    } else {
      Serial.println(F("[Boot] Memory preset id invalid"));
    }
  } else {
    Serial.println(F("[Boot] No valid preset in memory"));
  }

  metrics.begin(STEPS_PER_REV);
  stepper1.attachMetrics(&metrics, MotorId::Motor1);
  stepper2.attachMetrics(&metrics, MotorId::Motor2);
  steppers.begin(STEPS_PER_REV);
  Serial.println(F("[Boot] Steppers initialized"));

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("[Boot] IR ready"));

  bool wifiPrefLoaded = presetStore.loadWifiEnabled(wifiEnabledPref);
  if (!wifiPrefLoaded) wifiEnabledPref = false; // default OFF
  if (wifiEnabledPref) {
    wifiEnabledAt = millis();
    wifi.begin(this);
  } else {
    Serial.println(F("[WiFi] Disabled per preference"));
  }

  stepper1.testMotor();
  Serial.println(F("[Boot] Motor1 test done"));
  stepper2.testMotor();
  Serial.println(F("[Boot] Motor2 test done"));

  rgb.setColor(255, 0, 0);
  rgb.setOn(true);
  logStatus("Ready");
}

void WatchWinderApp::tick() {
  const unsigned long now = millis();
  steppers.stepTick(); // keep stepping first to avoid any latency
  handleIr(now);
  if (wifiEnabledPref && wifi.isEnabled()) {
    wifi.tick(now);
    if (wifiEnabledAt == 0) wifiEnabledAt = now;
    if (now - wifiEnabledAt >= WIFI_MAX_ON_MS) {
      wifi.disable();
      wifiEnabledPref = false;
      presetStore.saveWifiEnabled(false);
      logStatus("WiFi auto-disabled after 5 min");
      wifiEnabledAt = 0;
    }
  }
  presetRunner.tick(steppers, rgb, now);
  if (scheduleArmed && !presetRunner.isActive() && !manualRunning) {
    if (now - scheduleArmedAt >= scheduleDelayMs) {
      wifiStartPreset(schedulePresetId, now);
      scheduleArmed = false;
      scheduleRunCount++;
    }
  }
  handlePresetBlink(now);
  delay(0); // minimal yield
}

bool WatchWinderApp::wifiStartPreset(uint8_t presetId, unsigned long now) {
  const PresetConfig* preset = findPresetById(presetId);
  if (!preset) return false;
  selectPreset(preset);
  startSelectedPreset(now);
  return presetRunner.isActive();
}

void WatchWinderApp::wifiStopPreset() {
  stopPreset();
}

MotorSnapshot WatchWinderApp::wifiMotorSnapshot(MotorId id) {
  return metrics.snapshot(id);
}

bool WatchWinderApp::wifiSchedulePreset(uint8_t presetId, uint32_t delaySeconds) {
  const PresetConfig* preset = findPresetById(presetId);
  if (!preset) return false;
  scheduleArmed = true;
  schedulePresetId = presetId;
  scheduleDelayMs = delaySeconds * 1000UL;
  scheduleArmedAt = millis();
  Serial.print(F("[Schedule] Armed preset id="));
  Serial.print(presetId);
  Serial.print(F(" in "));
  Serial.print(delaySeconds);
  Serial.println(F("s"));
  return true;
}

bool WatchWinderApp::wifiCancelSchedule() {
  if (!scheduleArmed) return false;
  scheduleArmed = false;
  scheduleDelayMs = 0;
  schedulePresetId = 0;
  Serial.println(F("[Schedule] Cleared"));
  return true;
}

uint32_t WatchWinderApp::wifiScheduleSecondsRemaining(unsigned long now) const {
  if (!scheduleArmed) return 0;
  unsigned long elapsed = now - scheduleArmedAt;
  if (elapsed >= scheduleDelayMs) return 0;
  return (scheduleDelayMs - elapsed) / 1000UL;
}

bool WatchWinderApp::wifiManualStart(MotorSelection motors, StepperDir dir1, StepperDir dir2, StepperSpeed speed) {
  stopPreset(); // stop any preset or manual currently running
  manualRunning = true;
  manualMotors = motors;
  manualDir1 = dir1;
  manualDir2 = dir2;
  manualSpeed = speed;
  steppers.start(motors, dir1, speed, dir2);
  selectedPreset = nullptr;
  syncLedToState();
  Serial.print(F("[Manual] Start motors="));
  Serial.print(static_cast<uint8_t>(motors));
  Serial.print(F(" dir1="));
  Serial.print(dir1 == StepperDir::CW ? F("CW") : F("CCW"));
  Serial.print(F(" dir2="));
  Serial.print(dir2 == StepperDir::CW ? F("CW") : F("CCW"));
  Serial.print(F(" speed="));
  Serial.println(static_cast<uint8_t>(speed));
  return true;
}

void WatchWinderApp::wifiManualStop() {
  if (!manualRunning) return;
  steppers.stopAll();
  manualRunning = false;
  syncLedToState();
  Serial.println(F("[Manual] Stopped"));
}
