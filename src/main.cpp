#include <Arduino.h>
#include <IRremote.hpp>
#include "Arduino_LED_Matrix.h"
#include "IRRemoteMap.h"
#include "StepperController.h"
#include "RGBController.h"
#include "OledController.h"

// Pin mapping
constexpr uint8_t IR_PIN = 4;
constexpr uint8_t R_PIN = 6;  // PWM
constexpr uint8_t G_PIN = 5;  // PWM
constexpr uint8_t B_PIN = 3;  // PWM

constexpr uint8_t STP_IN1 = 7;
constexpr uint8_t STP_IN2 = 8;
constexpr uint8_t STP_IN3 = 9;
constexpr uint8_t STP_IN4 = 10;

// Motion and UI tuning
constexpr uint32_t STEPS_PER_REV = 4096UL;
constexpr uint32_t CONTINUOUS_TIMEOUT_MS = 10UL * 60UL * 1000UL; // 10 minutes
enum class AppState { IDLE, CONTINUOUS_LEFT, CONTINUOUS_RIGHT };
enum class StatusCode { STOPPED, RUNNING };
enum class DirectionCode { NONE, CW, CCW };

ArduinoLEDMatrix matrix;
StepperController stepper(STP_IN1, STP_IN2, STP_IN3, STP_IN4);
RGBController rgb(R_PIN, G_PIN, B_PIN);
OledController oled;

static AppState appState = AppState::IDLE;
static StatusCode currentStatus = StatusCode::STOPPED;
static DirectionCode currentDirection = DirectionCode::NONE;

// continuous start time for timeout
static unsigned long continuousStartMillis = 0;

// Map enums to display text
const char *statusToDisplay(StatusCode status) {
  return (status == StatusCode::RUNNING) ? "Running" : "Stopped";
}

const char *directionToDisplay(DirectionCode dir) {
  switch (dir) {
    case DirectionCode::CW:  return "Clockwise";
    case DirectionCode::CCW: return "Counter Clockwise";
    default:                 return "--";
  }
}

void updateDashboardNow() {
  const int32_t rotationsTenth = (int32_t)(stepper.getPosition() * 10L) / (int32_t)STEPS_PER_REV;
  oled.updateDashboard(statusToDisplay(currentStatus), directionToDisplay(currentDirection), rotationsTenth);
}

void enterContinuousLeft(unsigned long now) {
  appState = AppState::CONTINUOUS_LEFT;
  currentStatus = StatusCode::RUNNING;
  currentDirection = DirectionCode::CCW;
  rgb.setColor(0, 255, 0); // green while moving
  rgb.setOn(true);
  stepper.startContinuous(StepperDir::CCW, StepperSpeed::FAST);
  continuousStartMillis = now;
  updateDashboardNow();
}

void enterContinuousRight(unsigned long now) {
  appState = AppState::CONTINUOUS_RIGHT;
  currentStatus = StatusCode::RUNNING;
  currentDirection = DirectionCode::CW;
  rgb.setColor(0, 255, 0); // green while moving
  rgb.setOn(true);
  stepper.startContinuous(StepperDir::CW, StepperSpeed::FAST);
  continuousStartMillis = now;
  updateDashboardNow();
}

void exitContinuous(StatusCode reason, unsigned long now) {
  appState = AppState::IDLE;
  stepper.stopContinuous();
  rgb.setColor(255, 0, 0); // red when stopped
  rgb.setOn(true);
  currentStatus = reason;
  updateDashboardNow();
}

void handleIRButton(IRButton btn, unsigned long now) {
  switch (btn) {
    case IRButton::BTN_LEFT:
      enterContinuousLeft(now);
      break;
    case IRButton::BTN_RIGHT:
      enterContinuousRight(now);
      break;
    case IRButton::BTN_OK:
      if (appState == AppState::CONTINUOUS_LEFT || appState == AppState::CONTINUOUS_RIGHT) {
        exitContinuous(StatusCode::STOPPED, now);
      } else {
        Serial.println(F("OK pressed (no continuous running)."));
        currentStatus = StatusCode::STOPPED;
        updateDashboardNow();
      }
      break;

    // numeric commands 1..6 - blocking moves; ignore if continuous active
    case IRButton::BTN_1:
    case IRButton::BTN_2:
    case IRButton::BTN_3:
    case IRButton::BTN_4:
    case IRButton::BTN_5:
    case IRButton::BTN_6:
    {
      if (appState != AppState::IDLE) {
        Serial.println(F("Numeric command ignored during continuous motion."));
        break;
      }
      rgb.setColor(0, 255, 0); // green while moving
      rgb.setOn(true);
      currentStatus = StatusCode::RUNNING;
      currentDirection = (btn == IRButton::BTN_1 || btn == IRButton::BTN_2 || btn == IRButton::BTN_3)
                         ? DirectionCode::CCW : DirectionCode::CW;
      updateDashboardNow();

      switch (btn) {
        case IRButton::BTN_1: stepper.rotate(StepperDir::CCW, STEPS_PER_REV / 8U, StepperSpeed::SLOW);   break;
        case IRButton::BTN_2: stepper.rotate(StepperDir::CCW, STEPS_PER_REV / 4U, StepperSpeed::NORMAL); break;
        case IRButton::BTN_3: stepper.rotate(StepperDir::CCW, STEPS_PER_REV / 2U, StepperSpeed::FAST);   break;
        case IRButton::BTN_4: stepper.rotate(StepperDir::CW,  STEPS_PER_REV / 8U, StepperSpeed::SLOW);   break;
        case IRButton::BTN_5: stepper.rotate(StepperDir::CW,  STEPS_PER_REV / 4U, StepperSpeed::NORMAL); break;
        case IRButton::BTN_6: stepper.rotate(StepperDir::CW,  STEPS_PER_REV / 2U, StepperSpeed::FAST);   break;
        default: break;
      }

      rgb.setColor(255, 0, 0); // red when stopped
      rgb.setOn(true);
      currentStatus = StatusCode::STOPPED;
      updateDashboardNow();
    } break;

    default:
      Serial.println(F("Ignored button (not implemented)"));
      break;
  }
}

void processIr(unsigned long now) {
  if (!IrReceiver.decode()) {
    return;
  }
  const uint32_t raw = IrReceiver.decodedIRData.decodedRawData;
  const IRButton btn = IRRemoteMap::getButton(raw);
  Serial.print(F("BTN: "));
  Serial.println(IRRemoteMap::toString(btn));
  handleIRButton(btn, now);
  IrReceiver.resume();
}

void handleContinuousTimeout(unsigned long now) {
  if (appState == AppState::IDLE) {
    return;
  }
  if (now - continuousStartMillis >= CONTINUOUS_TIMEOUT_MS) {
    // Stop motion silently; user will refresh display on next OK press.
    appState = AppState::IDLE;
    stepper.stopContinuous();
    rgb.setColor(255, 0, 0); // red when stopped
    rgb.setOn(true);
    currentStatus = StatusCode::STOPPED;
  }
}

void setup() {
  Serial.begin(9600);
  delay(50);

  // Keep I2C slow to reduce SH1106/SSD1306 artifacts on long/noisy runs.
  Wire.setClock(80000);

  if (matrix.begin()) {
    Serial.println(F("Matrix init OK"));
  } else {
    Serial.println(F("Matrix not present or init failed"));
  }

  if (oled.begin()) {
    Serial.println(F("OLED init OK"));
  } else {
    Serial.println(F("OLED init failed"));
  }

  rgb.begin();
  rgb.setColor(255, 255, 0); // yellow while starting
  rgb.setOn(true);
  currentStatus = StatusCode::STOPPED;
  currentDirection = DirectionCode::NONE;
  updateDashboardNow();

  stepper.begin();
  stepper.setStepsPerRevolution(STEPS_PER_REV);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("IR ready"));

  // hardware test (blocking small sweep). This will update position.
  stepper.testMotor();
  delay(200);

  // set ready
  rgb.setColor(255, 0, 0); // red when ready/stopped
  rgb.setOn(true);
  currentStatus = StatusCode::STOPPED;
  appState = AppState::IDLE;
  continuousStartMillis = 0;
  updateDashboardNow();
}

void loop() {
  const unsigned long now = millis();
  processIr(now);

  // drive stepper (non-blocking)
  stepper.stepTick();

  handleContinuousTimeout(now);

  // keep loop fast and non-blocking
  delay(1);
}
