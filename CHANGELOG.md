# Changelog

## 2.1.0

### Fixed
- **Every sketch failed to compile on AVR (UNO/Nano/Mega)** with `ArduinoJson.h: No such file or directory`, even `LED_Blink`. The smart-home device registry was compiled on all boards; it is now ESP-only.
- **Missing dependencies.** `library.properties` now declares `depends=` (DHT sensor library, Adafruit Unified Sensor, ArduinoJson, WebSockets), so the Library Manager installs them. Previously users hit `DHT.h: No such file or directory`.
- `ActiveBuzzer::onFor()` / `offFor()` / `pulse()` never turned the buzzer back off: its `tick()` skipped `DigitalOutput::tick()`, which runs those timers.
- `PushButton` with `pullup = false` (button to VCC with a pull-down) reported presses inverted; `pressed()`, `released()` and `isPressed()` now follow the wiring.
- `keywords.txt` used spaces instead of tabs, so the Arduino IDE ignored it; added the methods the builder generates (`bind`, `isPressed`, `onFor`, …).

### Documented
- The `WiFi` module and the smart-home hub are experimental and must be enabled with build flags (`-DATG_ENABLE_WIFI=1`, `-DATG_SMARTHOME_ONLY=1`): a `#define` in the sketch never reaches the library's `.cpp` files and leads to link errors. Their behaviour is unchanged from 2.0.1.

### Removed
- `ATG_License` — unused anywhere in the library, and a key check whose secret ships in open source cannot protect anything.
- Duplicate `smarthome/ATG_RelayDevice.h` (broken include path; `devices/ATG_RelayDevice.h` is the real one), empty `ATG_DHT.cpp`, stray `debug.log`, `.vscode/`, and the duplicate `github/workflows/release.yml`.

### Added
- **New components** (each with an example, tested on a simulated ATmega328P running the compiled firmware):
  - `AnalogSensor` / `Potentiometer` — smoothed analog reading, `value()`/`percent()`/`ratio()`, normalised to 0–1023 on ESP32's 12-bit ADC too. Covers potentiometers, soil moisture, gas (MQ-x), water level and rain sensors.
  - `LightSensor` — LDR modules, `lightPercent()`/`isDark()`.
  - `Ultrasonic` — HC-SR04, `distanceCm()`/`inRange()`.
  - `DigitalSensor` — touch pads, tilt and limit switches, digital sound sensors, with `active()`/`activated()`.
  - `ServoMotor` (separate header) — on/off/toggle/onFor angles plus `setLevel()`.
  - `DCMotor` — H-bridge motor with speed (PWM) and direction, `setLevel()` for knob control.
  - `RGBLed` — colour, on/off/toggle and brightness via `setLevel()`; common anode or cathode.
  - `StepperMotor` — non-blocking 28BYJ-48/ULN2003 driver (continuous rotation, absolute moves, `setLevel()` position), unlike the blocking Arduino `Stepper` library.
  - `OledDisplay` — self-contained SSD1306 text-mode driver (128×64 / 128×32, 2× text, `°`), ~260 B RAM, sends only changed cells.
  - `BMP280Sensor` — self-contained BMP280 driver with Bosch's integer compensation, `altitudeM()`.
  - `MPU6050Sensor` — self-contained MPU-6050 driver: tilt, acceleration, gyro, `shaken()`.
  - `LED::setLevel(0..1)` for dimming (respects active-LOW wiring).
- Examples: `Potentiometer_Dimmer`, `Ultrasonic_ParkingSensor`, `Servo_Knob`, `LightSensor_NightLight`, `DigitalSensor_Touch`, `DCMotor_Knob`, `RGB_ColorToggle`, `Stepper_Knob`, `OLED_Thermometer`, `BMP280_Barometer`, `MPU6050_TiltAlarm`.
- CI: Arduino Lint (Library Manager rules) and compilation of every example for UNO, Nano, ESP32 and ESP8266 with only the declared dependencies.
- `architectures=avr,esp32,esp8266` and `includes=ATGENXlib.h` in `library.properties`.
- `extras/udp_discovery_test.py` (moved from the repository root).
- Rewritten README with accurate API, board matrix and verified examples.

## 2.0.1
- WiFi provisioning portal, UDP discovery, WebSocket bus.

## 2.0.0
- Runtime, scheduler, event bus and component architecture.
