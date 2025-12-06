# WatchWinder (UNO R4 WiFi)

Dual-stepper watch winder for the chronically forgetful. Runs on an Arduino UNO R4 WiFi, takes orders from an IR remote, flashes a smug little message on the onboard LED matrix, and blinks an RGB LED to let you know it is absolutely working very hard.

## Demo (action shots or it did not happen)
<video src="demo/video.mp4" controls width="640" muted></video>

If your markdown viewer ignores video tags, grab the file directly: `demo/video.mp4`.

## Hardware
- Arduino UNO R4 WiFi (Renesas RA)
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
| 7      | Both    | CW        | Duty: 10 min run / 15 min rest     | Purple @ 100%          |
| 8      | Both    | CCW       | Duty: 10 min run / 15 min rest     | Purple @ 50%           |

### Utility buttons
- `9` Save last running preset to EEPROM; flashes blue/red 3x on success or 1x if nothing to save.
- `0` Load saved preset from EEPROM; flashes blue/red 3x on success, 1x on failure.
- `#` System check: RGB LED color sweep, then CW/CCW rotation test on both motors (tiny flex).
- `*` Reset MCU (the Arduino equivalent of “turn it off and on again”).
- `UP/DOWN/LEFT/RIGHT` Reserved (no-op).

## Runtime behavior
- Boot: serial at 9600 baud, LED warmup (yellow), matrix init, and stepper self-test. Tries to load the last saved preset; on success the LED flashes pink, the matrix shows an "M," and that preset is selected. Cute.
- Matrix: shows the selected preset digit. Duty-cycle presets turn the LED off during rest windows, because even LEDs deserve breaks.
- Safety: continuous presets stop after 10 minutes if left running. Stopping a preset returns the LED to solid red, the universal color for “I’m done now.”
- Positioning: steppers use 4096 half-steps per revolution (28BYJ-48 default gearing). Math optional; trust the firmware.

## Build and flash
1. Install VS Code + PlatformIO. Coffee recommended.
2. Open this folder in PlatformIO. The environment is `uno_r4_wifi` with dependencies declared in `platformio.ini`:
   - Arduino-IRremote
   - ArduinoGraphics
   - IRRemoteMap
3. Build: `pio run`
4. Upload: `pio run -t upload`
5. Monitor (optional): `pio device monitor -b 9600`

## Notes
- IR button codes are mapped via the `IRRemoteMap` library; adjust that library's key map if your remote differs.
- The RGB LED is brightness-scaled per preset; it is off during duty-cycle rests and blinks blue/red for save/load feedback.

## License (permission required)
This project is not open-licensed. Any use, modification, distribution, or commercial activity requires prior written permission from the copyright holder(s). See `LICENSE.txt` for details.
