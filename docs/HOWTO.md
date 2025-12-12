# WatchWinder Build Guide

Step-by-step instructions to assemble, wire, build, and flash the WatchWinder firmware on supported Arduino boards (UNO R4 WiFi, UNO R3, or Nano).

## 1) Gather parts
- Board: UNO R4 WiFi (preferred for the LED matrix), or UNO R3, or Arduino Nano (ATmega328P). UNO R3/Nano run the same firmware but skip the LED matrix visuals.
- 2x 28BYJ-48 steppers + 2x ULN2003 driver boards
- 1x IR receiver module
- 1x common-cathode RGB LED + suitable resistors
- IR remote compatible with the `IRRemoteMap` layout (adjustable if needed)
- Optional: 3D-printed enclosure such as https://github.com/mwood77/osww
- Optional: WiFi (UNO R4 WiFi) — credentials in `include/WifiSecrets.h`, WiFi toggled via IR `UP`/`DOWN`, off by default.

## 2) Wire everything
All boards share the same pin map. Digital numbers refer to the Arduino silk-screen labels.

- IR receiver: `2`
- RGB LED (PWM): `R=6`, `G=5`, `B=3`
- Stepper 1: `IN1=7`, `IN2=8`, `IN3=18 (SDA/A4)`, `IN4=19 (SCL/A5)`
- Stepper 2: `IN1=4`, `IN2=9`, `IN3=10`, `IN4=11`
- UNO R4 WiFi only: onboard LED matrix used for preset digits and memory indicator. UNO R3/Nano simply skip matrix output.

Keep stepper wiring consistent between drivers so CW/CCW match the presets.

## 3) Install the toolchain
1) Install VS Code and the PlatformIO extension.
2) Clone or download this repo and open the folder in VS Code/PlatformIO.
3) Let PlatformIO install the required platforms and libraries automatically on first build.

## 4) Pick the correct PlatformIO environment
- UNO R4 WiFi: `uno_r4_wifi`
- UNO R3: `uno_r3`
- Arduino Nano (ATmega328P): `nano`

Set the default in the PlatformIO status bar or pass `-e <env>` to commands.

## 5) Build, upload, and monitor
From the project root:
```sh
pio run -e <env>            # build
pio run -e <env> -t upload  # flash
pio device monitor -b 9600  # optional serial monitor
```
Replace `<env>` with one of the environments above. The default is `uno_r4_wifi`.

## 6) Verify the firmware
- On boot you should see serial logs at 9600 baud. UNO R4 WiFi will also animate the LED matrix; UNO R3/Nano will log that the matrix is skipped.
- Press `#` on the remote for the self-test (RGB sweep + CW/CCW test on both steppers).
- Use preset buttons 1-8 to select and run the modes described in `README.md`. `OK` toggles run/stop. `9` saves the last running preset to EEPROM; `0` restores it.
- WiFi dashboard: runs only when WiFi is enabled via IR `UP` (stored in EEPROM). Data loads on refresh; there is no auto-poll to keep motors smooth.

## 7) Adapting to other remotes
IR button codes are mapped through the `IRRemoteMap` library (pulled from GitHub). If your remote differs, update the key map in that library and rebuild.

## 8) Troubleshooting
- Build fails on UNO R3/Nano about `Arduino_LED_Matrix`: ensure you selected `uno_r3` or `nano`; the R4-only matrix dependency is not used for AVR boards.
- Motors stutter: double-check ULN2003 wiring order and supply voltage. The firmware assumes 4096 half-steps per revolution.
- No IR input: confirm the receiver is on pin 2 and powered; check the serial log for `[IR]` entries when pressing buttons.

Happy winding.
