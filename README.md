# WatchWinder

Dual-stepper watch winder for the chronically forgetful. Runs on an Arduino UNO R4 WiFi (with LED matrix output) and now also builds on UNO R3 and Arduino Nano boards (matrix output is skipped on AVR boards). Takes orders from an IR remote and blinks an RGB LED to let you know it is absolutely working very hard.

## Demo (action shots or it did not happen)
<video src="demo/video.mp4" controls width="640" muted></video>

If your markdown viewer ignores video tags, grab the file directly: `demo/video.mp4`.

## Hardware
- Arduino UNO R4 WiFi (Renesas RA) — preferred for onboard LED matrix
- Arduino UNO R3 or Arduino Nano (ATmega328P) — same pin map; matrix output is skipped
- Two 28BYJ-48 steppers with ULN2003 driver boards
- IR receiver module
- Common-cathode RGB LED (PWM on R/G/B)
- Optional: 3D-printed case such as https://github.com/mwood77/osww

### Pin map (because smoke tests are overrated)
- IR receiver: `2`
- RGB LED: `R=6`, `G=5`, `B=3`
- Stepper 1 (existing): `IN1=7`, `IN2=8`, `IN3=18 (SDA)`, `IN4=19 (SCL)`
- Stepper 2 (new): `IN1=4`, `IN2=9`, `IN3=10`, `IN4=11`
- LED matrix: onboard UNO R4 matrix via `Arduino_LED_Matrix`

## Remote controls and presets
`OK` toggles start/stop for the selected preset. If no preset is selected when `OK` is pressed, the RGB LED blinks red/blue until you pick something—peer pressure, but for hardware.

### Preset buttons (1-8)
| Button | Motors  | Direction | Mode                               | LED color @ brightness |
| :----: | :------ | :-------- | :--------------------------------- | :--------------------- |
| 1      | Motor 1 | CW        | Continuous                         | Blue @ 100%            |
| 2      | Motor 2 | CW        | Continuous                         | Yellow @ 100%          |
| 3      | Both    | CW        | Continuous                         | Green @ 100%           |
| 4      | Motor 1 | CCW       | Continuous                         | Blue @ 50%             |
| 5      | Motor 2 | CCW       | Continuous                         | Yellow @ 50%           |
| 6      | Both    | CCW       | Continuous                         | Green @ 50%            |
| 7      | Both    | Alternates CW↔CCW | Duty: 10 min run / 10 min rest (direction flips each run) | White @ 50% |
| 8      | Both    | M1 CW / M2 CCW    | Duty: 10 min run / 10 min rest (both run together)        | Yellow @ 50%|

### Utility buttons
- `9` Save last running preset to EEPROM; flashes blue/red 3x on success or 1x if nothing to save.
- `0` Load saved preset from EEPROM; flashes blue/red 3x on success, 1x on failure.
- `#` System check: RGB LED color sweep, then CW/CCW rotation test on both motors (tiny flex).
- `*` Reset MCU after parking both motors back to logical zero.
- `UP/DOWN/LEFT/RIGHT` Reserved (no-op).

## Runtime behavior
- Boot: serial at 9600 baud, LED warmup (yellow), matrix init, and stepper self-test. Tries to load the last saved preset; on success the LED flashes pink, the matrix shows an "M," and that preset is selected. Cute.
- Matrix: shows the selected preset digit. Duty-cycle presets turn the LED off during rest windows, because even LEDs deserve breaks.
- Matrix orientation: if the board is mounted upside down, set `MATRIX_ORIENTATION` to `MatrixOrientation::UpsideDown` in `src/WatchWinderApp.h`.
- Safety: continuous presets stop after 10 minutes if left running. Stopping a preset returns the LED to solid red, the universal color for “I’m done now.”
- Positioning: steppers use 4096 half-steps per revolution (28BYJ-48 default gearing). Math optional; trust the firmware.

## Build and flash
1. Install VS Code + PlatformIO. Coffee recommended.
2. Open this folder in PlatformIO. Pick the environment for your board:
   - `uno_r4_wifi` (default) — includes the Arduino LED matrix dependency
   - `uno_r3` — AVR UNO R3, matrix skipped
   - `nano` — AVR Nano (ATmega328P), matrix skipped
3. Build: `pio run -e <env>`
4. Upload: `pio run -e <env> -t upload`
5. Monitor (optional): `pio device monitor -b 9600`
6. Full step-by-step instructions live in `docs/HOWTO.md`.

## Notes
- IR button codes are mapped via the `IRRemoteMap` library; adjust that library's key map if your remote differs.
- The RGB LED is brightness-scaled per preset; it is off during duty-cycle rests and blinks blue/red for save/load feedback.
