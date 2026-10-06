# ATGENXlib

[![CI](https://github.com/ARDUTECH0/ATGENXlib-0.0.1/actions/workflows/ci.yml/badge.svg)](https://github.com/ARDUTECH0/ATGENXlib-0.0.1/actions/workflows/ci.yml)
[![Arduino Library](https://www.ardu-badge.com/badge/ATGENXlib.svg)](https://www.arduinolibraries.info/libraries/atgen-xlib)

**Non-blocking components for Arduino & ESP32** — debounced buttons with multi-click, potentiometers and analog sensors, ultrasonic distance, light, motion/flame/IR/reed/touch/tilt sensors, DHT11/22, LEDs (dimmable), RGB LEDs, DC motors, steppers, servos, relays and buzzers — all driven by one small runtime, so your `loop()` never needs `delay()`.

ATGENXlib is the library behind the [ATGenX visual builder](https://atgenx.com): the code the builder generates uses it, and so can your own sketches.

---

## Install

**Arduino IDE:** *Tools → Manage Libraries…* → search **ATGENXlib** → *Install* → accept **Install all** (dependencies).

**arduino-cli:**

```bash
arduino-cli lib install ATGENXlib
```

Dependencies (installed automatically by the Library Manager): `DHT sensor library`, `Adafruit Unified Sensor`, `ArduinoJson`, `WebSockets`, `Servo`, `ESP32Servo`.
On AVR boards only the two DHT libraries are compiled in — plus `Servo` if you include the servo header.

## Supported boards

| Board | Components & runtime | WiFi portal | Smart-home hub |
| --- | :---: | :---: | :---: |
| Arduino UNO / Nano (ATmega328P) | ✅ | — | — |
| Arduino Mega 2560 | ✅ | — | — |
| ESP8266 (NodeMCU, Wemos D1) | ✅ | ✅ | — |
| ESP32 | ✅ | ✅ | 🧪 experimental |

Every example is compiled for UNO, Nano, ESP32 and ESP8266 on each commit (see CI).

---

## Quick start

```cpp
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

LED led(13);
PushButton btn(2);          // button between pin 2 and GND (internal pull-up)

void setup() {
  Serial.begin(115200);
  rt.attachLogger(&sink);
  rt.addModule(led);
  rt.addModule(btn);
  rt.begin();
}

void loop() {
  rt.loopOnce();            // updates every module — never blocks
  if (btn.pressed()) led.toggle();
}
```

### Multi-click, long press and timed outputs

```cpp
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
LED lamp(9);
Relay1Ch pump(8);           // most relay modules are active-LOW (the default)
PushButton btn(2);

void setup() {
  rt.addModule(lamp);
  rt.addModule(pump);
  rt.addModule(btn);

  btn.bind(1, [] { lamp.toggle(); });          // single click
  btn.bind(2, [] { pump.onFor(10000); });      // double click → pump on for 10 s
  btn.onLongPress([] { lamp.off(); pump.off(); });

  rt.begin();
}

void loop() {
  rt.loopOnce();
}
```

### Sensors

```cpp
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
DHTSensor room(4, DhtType::DHT22, 2000);   // pin, type, sample period (ms)
PIRMotion pir(5);
ActiveBuzzer buzzer(6);
Relay1Ch fan(8);

void setup() {
  Serial.begin(115200);
  rt.addModule(room);
  rt.addModule(pir);
  rt.addModule(buzzer);
  rt.addModule(fan);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  if (room.temperatureC() > 30) fan.on(); else fan.off();
  if (pir.motionStarted()) buzzer.beep(100, 100, 3);
}
```

---

## Components

| Class | Constructor | Main API |
| --- | --- | --- |
| `PushButton` | `(pin, pullup = true, debounceMs = 35, multiClickMs = 450, longPressMs = 650)` | `isPressed()` `pressed()` `released()` `bind(clicks, fn)` `onClicks()` `onLongPress()` `onPress()` `onRelease()` |
| `PIRMotion` | `(pin, debounceMs)` | `motion()` `motionStarted()` `motionEnded()` |
| `FlameDigital` | `(pin, activeLow, debounceMs)` | `detected()` `started()` |
| `IrObstacle` | `(pin, activeLow, debounceMs)` | `detected()` |
| `ReedSwitch` | `(pin, pullup, debounceMs)` | `closed()` |
| `SoundDigital` | `(pin, activeHigh, debounceMs)` | `triggered()` `pulse()` |
| `DHTSensor` | `(pin, DhtType::DHT11 \| DHT22, sampleMs = 2000)` | `temperatureC()` `humidity()` |
| `AnalogSensor` / `Potentiometer` | `(pin, sampleMs = 20)` | `value()` 0–1023 · `percent()` · `ratio()` 0–1 (smoothed; same scale on ESP32) |
| `LightSensor` | `(pin, brightIsHigh = false)` | `lightPercent()` · `lightRatio()` · `isDark(threshold = 30)` |
| `Ultrasonic` | `(trigPin, echoPin, sampleMs = 100, maxCm = 400)` | `distanceCm()` · `inRange()` · `ratio()` |
| `DigitalSensor` | `(pin, activeLow = false, pullup = false)` | `active()` `activated()` `deactivated()` — touch, tilt, limit switch, sound |
| `LED` | `(pin, activeLow = false)` | `on()` `off()` `toggle()` `onFor(ms)` `setBrightness(0–255)` `state()` |
| `Relay1Ch` | `(pin, activeLow = true)` | `on()` `off()` `toggle()` `onFor(ms)` `state()` |
| `ActiveBuzzer` | `(pin, activeLow = false)` | `beep(onMs, offMs, times)` `on()` `off()` |
| `ServoMotor` ¹ | `(pin, onAngle = 90, offAngle = 0)` | `on()` `off()` `toggle()` `onFor(ms)` `write(angle)` `setLevel(0–1)` |
| `DCMotor` | `(enPin, in1Pin, in2Pin = ATG_NO_PIN, speed = 255)` | `on()` `off()` `toggle()` `onFor(ms)` `setSpeed(0–255)` `setLevel(0–1)` `forward()` `reverse()` |
| `RGBLed` | `(r, g, b, commonAnode = false, red = 255, green = 255, blue = 255)` | `setColor(r, g, b)` `on()` `off()` `toggle()` `onFor(ms)` `setLevel(0–1)` |
| `OledDisplay` ² | `(address = 0x3C, height = 64, refreshMs = 250)` | `onRender(fn)` · `print(col, row, text, size)` · `print(col, row, value, decimals, unit, size)` — 16×8 text grid, 2× text, `°` |
| `BMP280Sensor` ² | `(address = 0x76, sampleMs = 1000)` | `temperatureC()` `pressureHpa()` `altitudeM()` `ok()` |
| `MPU6050Sensor` ² | `(address = 0x68, sampleMs = 20)` | `tiltX()` `tiltY()` `accelX/Y/Z()` `gyroX/Y/Z()` `shaken()` `ok()` |
| `StepperMotor` | `(in1, in2, in3, in4, stepsPerRev = 2048, rpm = 10)` | `on()` (rotate) `off()` (stop, coils off) `moveTo(steps)` `move(steps)` `setLevel(0–1)` (position over one turn) — never blocks `loop()` |

² I2C parts have their own headers (they pull in `Wire`): `components/displays/ATG_OledDisplay.h`, `components/sensors/ATG_BMP280.h`, `components/sensors/ATG_MPU6050.h`. They need no extra libraries. `OledDisplay` is a text-mode SSD1306 driver using ~260 bytes of RAM instead of the 1 KB frame buffer graphics libraries allocate — that's what lets a screen, sensors and the runtime fit on an UNO together.

¹ `#include <components/actuators/ATG_ServoMotor.h>` — kept out of `ATGENXlib.h` because the Servo library takes over a timer (on UNO it disables PWM on pins 9 and 10).

`LED` also has `setLevel(0–1)` for dimming, e.g. `lamp.setLevel(knob.ratio());`.

All inputs are debounced and report edges; all outputs handle active-LOW wiring for you, so `on()` always means *on*.

### Naming

Never name a variable after its class — `LED LED(13);` does not compile. Use `LED led(13);`.

---

## Configuration

Define these **before** `#include <ATGENXlib.h>` to change the defaults (see `src/core/ATG_Config.h`):

| Macro | Default | Meaning |
| --- | --- | --- |
| `ATG_LOG_LEVEL` | `2` | 0 off · 1 errors · 2 info · 3 debug |
| `ATG_MAX_MODULES` | `24` | modules a `Runtime` can hold |
| `ATG_MAX_TASKS` | `16` | scheduler tasks |
| `ATG_MAX_SUBSCRIBERS` | `32` | event-bus subscribers |
| `ATG_MAX_EVENTS_QUEUE` | `16` | queued events |

The WiFi provisioning portal (`WiFiPortal`, see the `WiFi_Portal` example) works out of the box on ESP32/ESP8266.

**Experimental modules** — the `WiFi` connection module and the smart-home hub — are compiled only when enabled with a **build flag**, not a `#define` in the sketch (library `.cpp` files never see sketch defines):

```bash
arduino-cli compile --fqbn esp32:esp32:esp32   --build-property "build.extra_flags=-DATG_ENABLE_WIFI=1 -DATG_SMARTHOME_ONLY=1" MySketch
```

(PlatformIO: `build_flags = -DATG_ENABLE_WIFI=1 -DATG_SMARTHOME_ONLY=1`.)

---

## Repository layout

```
ATGENXlib/
├─ src/
│  ├─ ATGENXlib.h                 single public include
│  ├─ core/                       Runtime, Module, Scheduler, EventBus, StateMachine, Log, Time, Config
│  ├─ boards/ATG_Board.h          board detection helpers
│  └─ components/
│     ├─ base/                    ComponentBase, DigitalInput, DigitalOutput
│     ├─ sensors/                 PushButton, PIR, DHT, Flame, Reed, IR obstacle, Sound
│     ├─ actuators/               LED, Relay1Ch, ActiveBuzzer
│     ├─ connectivity/            WiFiPortal, WiFi*, UDP discovery, WebSocket bus, HTTP API (ESP)
│     └─ smarthome/               device registry, manifest, hub* (ESP32)      * experimental, build flag
├─ examples/                      one sketch per component + demos
├─ extras/                        developer tools (UDP discovery tester)
├─ keywords.txt · library.properties · LICENSE · CHANGELOG.md
```

---

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## Star history

<a href="https://www.star-history.com/?repos=ARDUTECH0%2FATGENXlib-0.0.1&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/image?repos=ARDUTECH0/ATGENXlib-0.0.1&type=date&theme=dark&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/image?repos=ARDUTECH0/ATGENXlib-0.0.1&type=date&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/image?repos=ARDUTECH0/ATGENXlib-0.0.1&type=date&legend=top-left" />
 </picture>
</a>

## License

MIT — see [LICENSE](LICENSE).
