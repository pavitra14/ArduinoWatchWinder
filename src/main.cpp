#include <Arduino.h>
#include <IRremote.hpp>
#include "Arduino_LED_Matrix.h"
#include "IRRemoteMap.h"
#include "StepperController.h"
#include "RGBController.h"

// pins (same as before)
#define IR_PIN   2
#define R_PIN    6   // PWM
#define G_PIN    5   // PWM
#define B_PIN    3   // PWM

#define STP_IN1  8
#define STP_IN2  9
#define STP_IN3  10
#define STP_IN4  11

ArduinoLEDMatrix matrix;
StepperController stepper(STP_IN1, STP_IN2, STP_IN3, STP_IN4);
RGBController rgb(R_PIN, G_PIN, B_PIN);

const uint32_t STEPS_PER_REV = 4096;

// Continuous auto-stop timeout (10 minutes)
const unsigned long CONTINUOUS_TIMEOUT_MS = 10UL * 60UL * 1000UL; // 10 minutes

// application run-state
enum class AppState { IDLE, CONTINUOUS_LEFT, CONTINUOUS_RIGHT };
static AppState appState = AppState::IDLE;

// siren blink control
unsigned long lastBlinkMillis = 0;
const unsigned long BLINK_INTERVAL = 300; // ms between color swaps
bool sirenToggle = false;

// continuous start time for timeout
unsigned long continuousStartMillis = 0;

// matrix refresh control
unsigned long lastMatrixUpdateMillis = 0;
const unsigned long MATRIX_UPDATE_INTERVAL = 250; // refresh pos every 250ms
String currentStatus = "RDY"; // text shown on top line

// helper to draw 2-line static display: top status, bottom position
void updateMatrixStatusAndPos(const String &status, int32_t posSteps) {
  // Draw status (top) and static position (bottom). Coordinates may need slight tuning depending on your matrix.
  matrix.beginDraw();
  matrix.clear(); // be explicit
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(100);
  matrix.textFont(Font_4x6);

  // Top line: status at y=0
  matrix.beginText(0, 0, 0xFFFFFFFF);
  matrix.print(status);
  matrix.endText(NO_SCROLL);

  // Bottom line: position. Show raw half-steps and revolutions approx.
  char buf[32];
  // show steps and revolutions with 2 decimal places
  float revs = (float)posSteps / (float)STEPS_PER_REV;
  // format: "POS: +1234  R:0.30"
  snprintf(buf, sizeof(buf), "POS:%ld R:%.2f", (long)posSteps, revs);

  // y coordinate chosen to appear on second line — tweak if needed for your matrix
  matrix.beginText(0, 9, 0xFFFFFFFF);
  matrix.print(String(buf));
  matrix.endText(NO_SCROLL);

  matrix.endDraw();
}

void showTextOnMatrix(const String &text) {
  // for single-line messages we still want the pos line below — so read position and call updateMatrixStatusAndPos
  updateMatrixStatusAndPos(text, stepper.getPosition());
}

// siren helpers
void setSirenStart() {
  lastBlinkMillis = millis();
  sirenToggle = false;
  rgb.setColor(255,0,0);
  rgb.setOn(true);
}

void updateSirenBlink() {
  unsigned long now = millis();
  if (now - lastBlinkMillis >= BLINK_INTERVAL) {
    lastBlinkMillis = now;
    sirenToggle = !sirenToggle;
    if (sirenToggle) rgb.setColor(0,0,255);
    else rgb.setColor(255,0,0);
    rgb.setOn(true);
  }
}

void enterContinuousLeft(){
  appState = AppState::CONTINUOUS_LEFT;
  currentStatus = "LEFT";
  updateMatrixStatusAndPos(currentStatus, stepper.getPosition());
  setSirenStart();
  stepper.startContinuous(StepperDir::CCW, StepperSpeed::FAST);
  continuousStartMillis = millis();
}

void enterContinuousRight(){
  appState = AppState::CONTINUOUS_RIGHT;
  currentStatus = "RIGHT";
  updateMatrixStatusAndPos(currentStatus, stepper.getPosition());
  setSirenStart();
  stepper.startContinuous(StepperDir::CW, StepperSpeed::FAST);
  continuousStartMillis = millis();
}

void exitContinuous(const char* reason = "RDY"){
  appState = AppState::IDLE;
  stepper.stopContinuous();
  rgb.setColor(0,255,0);
  rgb.setOn(true);
  currentStatus = String(reason);
  updateMatrixStatusAndPos(currentStatus, stepper.getPosition());
}

void handleIRButton(IRButton btn) {
  switch (btn) {
    case IRButton::BTN_LEFT:
      // start continuous CCW until OK
      enterContinuousLeft();
      break;
    case IRButton::BTN_RIGHT:
      // start continuous CW until OK
      enterContinuousRight();
      break;
    case IRButton::BTN_OK:
      // stop continuous if running
      if (appState == AppState::CONTINUOUS_LEFT || appState == AppState::CONTINUOUS_RIGHT) {
        exitContinuous("RDY");
      } else {
        Serial.println("OK pressed (no continuous running).");
      }
      break;

    // numeric commands 1..6 — blocking moves; ignore if continuous active
    case IRButton::BTN_1:
    case IRButton::BTN_2:
    case IRButton::BTN_3:
    case IRButton::BTN_4:
    case IRButton::BTN_5:
    case IRButton::BTN_6:
    {
      if (appState != AppState::IDLE) {
        Serial.println("Numeric command ignored during continuous motion.");
        break;
      }
      // set red and CMD
      rgb.setColor(255,0,0); rgb.setOn(true);
      currentStatus = "CMD";
      updateMatrixStatusAndPos(currentStatus, stepper.getPosition());

      if (btn == IRButton::BTN_1) stepper.rotate(StepperDir::CCW, STEPS_PER_REV/8, StepperSpeed::SLOW);
      if (btn == IRButton::BTN_2) stepper.rotate(StepperDir::CCW, STEPS_PER_REV/4, StepperSpeed::NORMAL);
      if (btn == IRButton::BTN_3) stepper.rotate(StepperDir::CCW, STEPS_PER_REV/2, StepperSpeed::FAST);
      if (btn == IRButton::BTN_4) stepper.rotate(StepperDir::CW,  STEPS_PER_REV/8, StepperSpeed::SLOW);
      if (btn == IRButton::BTN_5) stepper.rotate(StepperDir::CW,  STEPS_PER_REV/4, StepperSpeed::NORMAL);
      if (btn == IRButton::BTN_6) stepper.rotate(StepperDir::CW,  STEPS_PER_REV/2, StepperSpeed::FAST);

      rgb.setColor(0,255,0); rgb.setOn(true);
      currentStatus = "DONE";
      updateMatrixStatusAndPos(currentStatus, stepper.getPosition());
    } break;

    default:
      Serial.println("Ignored button (not implemented)");
      break;
  }
}

void setup() {
  Serial.begin(9600);
  delay(50);

  if (matrix.begin()) {
    Serial.println("Matrix init OK");
  } else {
    Serial.println("Matrix not present or init failed");
  }

  rgb.begin();
  rgb.setColor(255,0,0); // red while starting
  rgb.setOn(true);
  currentStatus = "WAIT";
  updateMatrixStatusAndPos(currentStatus, 0);

  stepper.begin();
  stepper.setStepsPerRevolution(STEPS_PER_REV);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR ready");

  // hardware test (blocking small sweep). This will update position.
  stepper.testMotor();
  delay(200);

  // set ready
  rgb.setColor(0,255,0); rgb.setOn(true);
  currentStatus = "RDY";
  updateMatrixStatusAndPos(currentStatus, stepper.getPosition());
  appState = AppState::IDLE;
  continuousStartMillis = 0;
}

void loop() {
  // process IR when available
  if (IrReceiver.decode()) {
    uint32_t raw = IrReceiver.decodedIRData.decodedRawData;
    IRButton btn = IRRemoteMap::getButton(raw);
    const char *name = IRRemoteMap::toString(btn);
    Serial.print("BTN: "); Serial.println(name);
    handleIRButton(btn);
    IrReceiver.resume();
  }

  // drive stepper (non-blocking)
  stepper.stepTick();

  // timeout check for continuous runs
  if (appState != AppState::IDLE) {
    unsigned long now = millis();
    if (now - continuousStartMillis >= CONTINUOUS_TIMEOUT_MS) {
      // auto-stop
      exitContinuous("TIMEOUT");
    }
  }

  // update siren blink & matrix while continuous
  if (appState == AppState::CONTINUOUS_LEFT || appState == AppState::CONTINUOUS_RIGHT) {
    updateSirenBlink();

    unsigned long now = millis();
    if (now - lastMatrixUpdateMillis >= MATRIX_UPDATE_INTERVAL) {
      lastMatrixUpdateMillis = now;
      // refresh the static display showing status and position
      updateMatrixStatusAndPos(currentStatus, stepper.getPosition());
    }
  } else {
    // if idle, update position less frequently to reflect any changes
    unsigned long now = millis();
    if (now - lastMatrixUpdateMillis >= 1000) {
      lastMatrixUpdateMillis = now;
      updateMatrixStatusAndPos(currentStatus, stepper.getPosition());
    }
  }

  // keep loop fast and non-blocking
  delay(1);
}
