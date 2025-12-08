# WatchWinder Dual-Stepper Spec

## Goals
- Drive two 28BYJ-48 steppers (ULN2003) from an Arduino UNO R4 WiFi with IR remote control.
- Modular, preset-based runtime that maps remote keys to motion, LED color/brightness, and status handling.
- Show the active preset on the UNO R4 LED matrix; expose serial logs for debugging.
- Persist the last running preset and allow save/load via remote buttons.

## Hardware Map
- IR receiver: `PIN 4`.
- RGB LED (common cathode): `R=6`, `G=5`, `B=3` (PWM).
- Stepper 1 (existing): `IN1=7`, `IN2=8`, `IN3=9`, `IN4=10`.
- Stepper 2 (new): `IN1=2`, `IN2=11`, `IN3=12`, `IN4=13`.
- LED matrix: onboard UNO R4 matrix via `Arduino_LED_Matrix`.

## Runtime Architecture
- `StepperController`: keeps current implementation for a single motor (position tracking, blocking and continuous modes).
- `DualStepperManager` (new): owns two `StepperController` instances; provides unified APIs to start/stop coordinated motion (single motor or both) and to tick both motors non-blockingly.
- `RGBController`: reused; add helpers for brightness + preset colors.
- `Preset` model:
  - Fields: id/button, label, motors target (motor1 only, motor2 only, both), direction (CW/CCW), mode (continuous vs timed-duty vs fixed-steps), duty cycle (`runMs`, `restMs`, `repeat`), LED color + brightness, matrix glyph.
  - Stored in a registry map keyed by `IRButton`.
- `PresetRunner` (new):
  - Drives execution of a selected preset; manages state (idle, running, paused), start/stop, timing windows for duty-cycle presets (10min on / 15min off loop).
  - Coordinates with `DualStepperManager` and `RGBController`.
- `PresetStore` (new): thin EEPROM wrapper to save/load the last preset id with checksum.
- `UI` layer:
  - Matrix: render preset label/short code when selected, and status when running/stopped.
  - Serial: emit concise status lines for debugging.

## Preset Definitions (per requirements)
- `BTN_1`: Motor1, CW, fixed continuous while active, LED Blue @100%.
- `BTN_2`: Motor2, CW, LED Yellow @100%.
- `BTN_3`: Both, CW, LED Green @100%.
- `BTN_4`: Motor1, CCW, LED Blue @50%.
- `BTN_5`: Motor2, CCW, LED Yellow @50%.
- `BTN_6`: Both, CCW, LED Green @50%.
- `BTN_7`: Both, alternating direction duty cycle: run 10min CW, rest 10min, run 10min CCW, rest 10min, repeat; LED White @50%.
- `BTN_8`: Both, opposing directions: Motor1 CW + Motor2 CCW, run 10min, rest 10min, repeat together; LED Yellow @50%.
- `BTN_9`: Save last running preset to EEPROM; flash LED Blue/Red x3 on success, x1 on failure.
- `BTN_0`: Load last preset from EEPROM; flash LED Blue/Red x3 on success, x1 on failure.
- `BTN_*`: Reset MCU (via `NVIC_SystemReset` or watchdog kick).
- `BTN_OK`: Start/Stop current preset. If none selected, flash LED Red/Blue continuously until selection.
- `BTN_#`: System check: rotate both motors CW one rev each, then CCW; flash full RGB spectrum quickly.
- `BTN_UP/DOWN/LEFT/RIGHT`: reserved empty presets (no-op for now).

## Behaviors
- Selecting a numeric preset updates matrix with preset label/number and applies LED color/brightness but does not move until `OK` is pressed (except system-test/reset/save/load).
- `OK` toggles run/pause of the active preset. Stopping returns LED to a neutral red.
- Continuous presets (1-6) run both motors in the requested direction until stopped.
- Duty-cycle presets (7-8) run `runMs` then pause `restMs` repeatedly while active; matrix should show a simple heartbeat tick or icon for run/rest.
- System timeout: keep current 10-minute safety cutoff for manual continuous modes; duty-cycle handles its own timing.
- Matrix display:
  - On select: show preset code (e.g., `P1`, `P2`, `P7`).
  - On run: show direction arrow (CW/CCW) or progress mark.
-  - On stop: show `STP` or last preset code.
- Matrix orientation: configurable via `MatrixOrientation` (Normal vs UpsideDown) in the app; use `UpsideDown` when the board is mounted inverted so digits render correctly.

## Persistence
- Store last successfully running preset id in EEPROM address `0` with a 1-byte checksum.
- `save`: invoked by BTN_9 while a preset is running.
- `load`: invoked by BTN_0; if valid, select preset and light success pattern.

## Tasks
1. Add new pin mapping for Stepper 2; instantiate second motor.
2. Introduce preset model/registry and matrix-friendly labels.
3. Implement `DualStepperManager` and `PresetRunner` to coordinate two motors and duty-cycle timing in loop.
4. Wire IR handlers to presets, start/stop, save/load, reset, and system check behaviors.
5. Update RGB controller usage for brightness per preset and status flash helpers (success/fail).
6. Render preset/status on LED matrix; keep serial logs minimal.
7. Validate EEPROM save/load routines; graceful handling when unset.
8. Document new behaviors in README after code stabilizes.
