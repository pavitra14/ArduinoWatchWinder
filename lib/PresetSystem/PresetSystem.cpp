#include "PresetSystem.h"
#include <EEPROM.h>

static const uint32_t TEN_MIN_MS = 10UL * 60UL * 1000UL;
static const uint32_t FIFTEEN_MIN_MS = 15UL * 60UL * 1000UL;
static const uint32_t TEN_MIN_WAIT_MS = TEN_MIN_MS;

// Color helpers
static constexpr RgbColor BLUE   = {0, 0, 255};
static constexpr RgbColor GREEN  = {0, 255, 0};
static constexpr RgbColor YELLOW = {255, 255, 0};
static constexpr RgbColor WHITE  = {255, 255, 255};
static constexpr RgbColor PURPLE = {160, 0, 160};

static const PresetConfig PRESETS[] = {
  {1, IRButton::BTN_1, "Motor1 CW 100",   MotorSelection::MOTOR1, StepperDir::CW,  StepperDir::CW,  false, StepperDir::CW,  PresetMode::CONTINUOUS, 0, 0, BLUE,   255, StepperSpeed::FAST},
  {2, IRButton::BTN_2, "Motor2 CW 100",   MotorSelection::MOTOR2, StepperDir::CW,  StepperDir::CW,  false, StepperDir::CW,  PresetMode::CONTINUOUS, 0, 0, YELLOW, 255, StepperSpeed::FAST},
  {3, IRButton::BTN_3, "Both CW 100",     MotorSelection::BOTH,   StepperDir::CW,  StepperDir::CW,  false, StepperDir::CW,  PresetMode::CONTINUOUS, 0, 0, GREEN,  255, StepperSpeed::FAST},
  {4, IRButton::BTN_4, "Motor1 CCW 50",   MotorSelection::MOTOR1, StepperDir::CCW, StepperDir::CCW, false, StepperDir::CCW, PresetMode::CONTINUOUS, 0, 0, BLUE,   128, StepperSpeed::FAST},
  {5, IRButton::BTN_5, "Motor2 CCW 50",   MotorSelection::MOTOR2, StepperDir::CCW, StepperDir::CCW, false, StepperDir::CCW, PresetMode::CONTINUOUS, 0, 0, YELLOW, 128, StepperSpeed::FAST},
  {6, IRButton::BTN_6, "Both CCW 50",     MotorSelection::BOTH,   StepperDir::CCW, StepperDir::CCW, false, StepperDir::CCW, PresetMode::CONTINUOUS, 0, 0, GREEN,  128, StepperSpeed::FAST},
  {7, IRButton::BTN_7, "Both alt CW/CCW duty", MotorSelection::BOTH, StepperDir::CW, StepperDir::CW, true,  StepperDir::CCW, PresetMode::DUTY_CYCLE, TEN_MIN_MS, TEN_MIN_WAIT_MS, WHITE, 128, StepperSpeed::FAST},
  {8, IRButton::BTN_8, "M1 CW / M2 CCW duty",  MotorSelection::BOTH, StepperDir::CW, StepperDir::CCW, false, StepperDir::CW, PresetMode::DUTY_CYCLE, TEN_MIN_MS, TEN_MIN_WAIT_MS, YELLOW, 128, StepperSpeed::FAST}
};

const PresetConfig* findPreset(IRButton button) {
  for (const auto &p : PRESETS) {
    if (p.button == button) return &p;
  }
  return nullptr;
}

const PresetConfig* findPresetById(uint8_t id) {
  for (const auto &p : PRESETS) {
    if (p.id == id) return &p;
  }
  return nullptr;
}

/* DualStepperManager ----------------------------------------------------- */
DualStepperManager::DualStepperManager(StepperController &m1, StepperController &m2)
: motor1(m1), motor2(m2) {}

void DualStepperManager::begin(uint32_t stepsPerRevolution) {
  motor1.begin();
  motor2.begin();
  motor1.setStepsPerRevolution(stepsPerRevolution);
  motor2.setStepsPerRevolution(stepsPerRevolution);
}

void DualStepperManager::start(MotorSelection target,
                               StepperDir dirMotor1,
                               StepperSpeed speed,
                               StepperDir dirMotor2) {
  stopAll();
  if (target == MotorSelection::MOTOR1 || target == MotorSelection::BOTH) {
    motor1.startContinuous(dirMotor1, speed);
  }
  if (target == MotorSelection::MOTOR2 || target == MotorSelection::BOTH) {
    motor2.startContinuous((target == MotorSelection::BOTH) ? dirMotor2 : dirMotor1, speed);
  }
}

void DualStepperManager::stopAll() {
  motor1.stopContinuous();
  motor2.stopContinuous();
}

void DualStepperManager::stepTick() {
  motor1.stepTick();
  motor2.stepTick();
}

void DualStepperManager::testSequential(uint32_t stepsPerRev) {
  motor1.rotate(StepperDir::CW, stepsPerRev, StepperSpeed::FAST);
  motor1.rotate(StepperDir::CCW, stepsPerRev, StepperSpeed::FAST);
  motor2.rotate(StepperDir::CW, stepsPerRev, StepperSpeed::FAST);
  motor2.rotate(StepperDir::CCW, stepsPerRev, StepperSpeed::FAST);
}

/* PresetRunner ----------------------------------------------------------- */
bool PresetRunner::start(const PresetConfig* preset,
                         DualStepperManager &steppers,
                         RGBController &rgb,
                         unsigned long nowMillis) {
  if (!preset) return false;
  activePreset = preset;
  phase = RunnerPhase::Running;
  phaseStart = nowMillis;
  currentDir = activePreset->direction;
  currentDirMotor2 = activePreset->motor2Direction;

  rgb.setBrightness(activePreset->brightness);
  rgb.setColor(activePreset->color.r, activePreset->color.g, activePreset->color.b);
  rgb.setOn(true);
  steppers.start(activePreset->motors, currentDir, activePreset->speed, currentDirMotor2);
  Serial.print(F("[PresetRunner] Start preset "));
  Serial.print(activePreset->id);
  Serial.print(F(" mode="));
  Serial.print(activePreset->mode == PresetMode::DUTY_CYCLE ? F("duty") : F("continuous"));
  Serial.print(F(" dir1="));
  Serial.print(currentDir == StepperDir::CW ? F("CW") : F("CCW"));
  if (activePreset->motors == MotorSelection::BOTH) {
    Serial.print(F(" dir2="));
    Serial.print(currentDirMotor2 == StepperDir::CW ? F("CW") : F("CCW"));
  }
  Serial.println();
  return true;
}

void PresetRunner::stop(DualStepperManager &steppers, RGBController &rgb) {
  steppers.stopAll();
  rgb.setColor(255, 0, 0);
  rgb.setOn(true);
  phase = RunnerPhase::Idle;
  activePreset = nullptr;
  Serial.println(F("[PresetRunner] Stop (idle)"));
}

void PresetRunner::resolveNextRunDirections(StepperDir &dir1Out, StepperDir &dir2Out) const {
  if (!activePreset) return;
  dir1Out = activePreset->direction;
  dir2Out = activePreset->motor2Direction;
  if (activePreset->alternateDirection) {
    const bool currentlyPrimary = (currentDir == activePreset->direction);
    dir1Out = currentlyPrimary ? activePreset->altDirection : activePreset->direction;
    dir2Out = dir1Out; // alternate both motors together
  }
}

void PresetRunner::tick(DualStepperManager &steppers, RGBController &rgb, unsigned long nowMillis) {
  if (phase == RunnerPhase::Idle || !activePreset) return;

  if (activePreset->mode == PresetMode::CONTINUOUS) {
    return;
  }

  if (activePreset->mode == PresetMode::DUTY_CYCLE) {
    if (phase == RunnerPhase::Running) {
      if (nowMillis - phaseStart >= activePreset->runMs) {
        steppers.stopAll();
        phase = RunnerPhase::Resting;
        phaseStart = nowMillis;
        rgb.setOn(false); // pause LED while resting
        Serial.println(F("[PresetRunner] Enter rest phase"));
      }
    } else if (phase == RunnerPhase::Resting) {
      if (nowMillis - phaseStart >= activePreset->restMs) {
        StepperDir nextDir1 = currentDir;
        StepperDir nextDir2 = currentDirMotor2;
        resolveNextRunDirections(nextDir1, nextDir2);
        currentDir = nextDir1;
        currentDirMotor2 = nextDir2;
        steppers.start(activePreset->motors, currentDir, activePreset->speed, currentDirMotor2);
        phase = RunnerPhase::Running;
        phaseStart = nowMillis;
        rgb.setOn(true);
        Serial.print(F("[PresetRunner] Enter run phase dir1="));
        Serial.print(currentDir == StepperDir::CW ? F("CW") : F("CCW"));
        if (activePreset->motors == MotorSelection::BOTH) {
          Serial.print(F(" dir2="));
          Serial.print(currentDirMotor2 == StepperDir::CW ? F("CW") : F("CCW"));
        }
        Serial.println();
      }
    }
  }
}

/* PresetStore ------------------------------------------------------------ */
static constexpr uint8_t EEPROM_ADDR_ID = 0;
static constexpr uint8_t EEPROM_ADDR_CSUM = 1;
static constexpr uint8_t CSUM_MAGIC = 0xA5;

bool PresetStore::save(uint8_t presetId) {
  uint8_t csum = presetId ^ CSUM_MAGIC;
  EEPROM.update(EEPROM_ADDR_ID, presetId);
  EEPROM.update(EEPROM_ADDR_CSUM, csum);
  return true;
}

bool PresetStore::load(uint8_t &presetIdOut) {
  uint8_t saved = EEPROM.read(EEPROM_ADDR_ID);
  uint8_t csum = EEPROM.read(EEPROM_ADDR_CSUM);
  if ((saved ^ CSUM_MAGIC) != csum) {
    return false;
  }
  if (saved == 0 || saved > 8) {
    return false;
  }
  presetIdOut = saved;
  return true;
}
