# Changelog

## 2.6.0

### Added
- **Phone**: `reply(word, answer)` — words of your own and what the board sends back: `phone.reply("حرارة", temperature)` answers "حرارة" with the temperature alone. Up to 8 words. Arabic is matched loosely (ال, ة/ه, أ/إ/آ, ى/ي, short vowels, ؟), English in any case, and a word inside a longer message counts ("الحرارة كام؟"). Only messages from the numbers given to `answer()` are answered, and the answer goes to that stored number.
- `answer(numbers…, nullptr)`: take questions for `reply()` words only, without the "STATUS" answer.

### Changed
- Three answers in any three minutes (was: one a minute), so a second question right after the first is answered too; still bounded, so a flood can't spend the SIM's credit.
- "حالة؟" and "STATUS!" count as the status question (punctuation and letter forms no longer matter).

## 2.5.0

### Added
- **Phone** (`#include <ATG_Phone.h>`, after `<TinyGsmClient.h>`): text messages and calls from a SIM board (LILYGO T-A7670X, T-SIM7600X, T-SIM7670G-S3, TTGO T-Call).
  - `ATG_Phone` sends one at a time from a queue, tries a refused message again (3 tries, 20 s apart), waits for the network when there's none (up to 10 minutes), rings each call 30 s, and sends Arabic messages (70 letters each; a longer text goes as several).
  - `onBusy(pause, resume)` pauses the board's internet around them: on these modems a message sent in the middle of the cloud connection drops it, and a call on a network without 4G voice moves the modem to 2G. `begin()` also asks for calls over 4G (`AT+CEVDP=3`) and keeps new messages off the serial line.
  - Messages are sent by `ATG_Phone` itself, not TinyGSM's `sendSMS_UTF16`: that one asks for `AT+CSCS="HEX"`, which only the SIM800 has — on the A76xx and SIM76xx an Arabic message never went — and it left the coding at UCS-2, so the next Latin message went garbled. Arabic now goes in UCS2 mode (the number too), Latin in the GSM alphabet, and the modem is put back after each one. A refused message prints the network's `+CMS ERROR`.
  - `answer(numbers…, status)`: a message saying "STATUS", "حالة" or "?" from one of the board's numbers gets the readings back — sent to the number stored on the board, never to the one the message shows (a sender's number can be faked), one answer a minute at most. Messages from other numbers are deleted.
  - `ATG_PhoneAlert` is the alert a rule switches: a message, a call or both to up to three numbers, once each time it's switched on and not again before its gap, the reading at the end of the message.
  - `#define ATG_PHONE_NO_CALLS` for a modem without voice (SIM7080G).
- **Logic** (`#include <ATG_Logic.h>`): the parts the rules on the board read and switch — `ATG_In` (debounced), `ATG_Out`, `ATG_Sonar`, `ATG_DhtReader` — and `atgxScale`, `atgxPercent`, `atgxNum`. The ATGenX Builder's sketches used to carry these inline.
- Example `Phone_Alerts`.

## 2.4.0

### Changed
- `StepperDriver` runs on **FastAccelStepper** when the sketch includes `<FastAccelStepper.h>` before `<ATG_Motion.h>`: the STEP pulses come from the chip's own timers (ESP32: any pin; UNO/Nano: pins 9 and 10; Mega: 6, 7 and 8), so the speed is steady and high whatever `loop()` is doing. Same API (`begin`, `update`, `speed`, `stepsLeft`, `moving`); `usesTimer()` tells which way it runs.
- A pin FastAccelStepper can't drive, a sketch without it, or `#define ATG_NO_FAST_STEPPER` (the ATGenX simulation, which watches each step) keeps stepping from `loop()` as before.
- On AVR, FastAccelStepper takes Timer1, which the Servo library needs too: a sketch with a servo should define `ATG_NO_FAST_STEPPER`.

## 2.3.0

### Added
- **Motion** (`#include <ATG_Motion.h>`, separate from `ATGENXlib.h` because the tilt sensor pulls in `Wire`):
  - `StepperDriver` — a stepper on any STEP/DIR driver (A4988, DRV8825, TMC2208/2209, TB6600, DM542T…). Speeds up and slows down at a set rate, slows to a stop before changing direction, and lands a counted move on its last step. Never blocks `loop()`.
  - `RobotBase` — two DC motors on one H-bridge (L298N, L293D, TB6612FNG), `drive(left, right)` from -255 to 255, and `update(run, throttle, steer)` for driving from an app.
  - `LineSensor` and `LineFollower` — up to five infrared eyes, the line's position from -100 to 100, and PID that follows it.
  - `TiltSensor` and `Balancer` — the MPU6050 as a tilt meter (gyro zeroed at power-on, complementary filter) and PID that keeps a two-wheeled robot standing.
  - Four examples: `Motion_StepperDriver`, `Motion_RobotCar`, `Motion_LineFollower`, `Motion_BalancingRobot`.
- These are the parts the ATGenX visual builder writes for steppers and robots. Each was tested as compiled firmware on a simulated ATmega328P against a model of the machine: a counted turn lands on step 3200 of 3200, the line follower holds the track, the balancing robot stands and recovers from a shove.

## 2.2.0

### Added
- **Raspberry Pi Pico (RP2040)**: board detection (`ATG_Board.h`), `architectures` now lists `rp2040`, and CI compiles every example for `rpipico`.
- Every example still compiles on UNO and ESP32.

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
