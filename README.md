# ATGENXlib

ATGENXlib is the official core library for the **ATGenX** visual Arduino/ESP32 builder platform. It provides a modular runtime engine, component abstraction layer, and production-ready sensor/actuator modules designed for code generation systems.

---

## ✨ Features

* Modular Runtime Engine
* Event Bus System
* Non-blocking Scheduler
* State Machine Engine
* Board Abstraction (UNO / ESP32)
* Digital Input with Debounce & Edge Detection
* Digital Output with Active-LOW support
* Non-blocking Buzzer Patterns
* DHT Temperature & Humidity Support
* Production-ready Component Structure

---

## 🧱 Supported Boards


| Board           | Status       |
| --------------- | ------------ |
| Arduino UNO R3  | ✅ Supported |
| ESP32 DevKit V1 | ✅ Supported |
| ESP32 D4 Pico   | ✅ Supported |

---

## 🔌 Supported Components

### Sensors

* DHT11 / DHT22
* Push Button (Pullup supported)
* PIR Motion Sensor
* Flame Sensor (Digital)
* Reed Switch
* IR Obstacle Sensor
* Sound Sensor (Digital)

### Actuators

* LED (PWM supported)
* Relay 1-Channel
* Active Buzzer (non-blocking patterns)

---

## 🚀 Quick Start Example

```cpp
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);
LED led(13);
PushButton btn(2);

void setup() {
  Serial.begin(115200);
  delay(200);
  rt.attachLogger(&sink);
  rt.addModule(led);
  rt.addModule(btn);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  if (btn.pressed()) {
    led.toggle();
  }
}
```

---

## 🧩 Provisioning Flow

ATGenX generates Arduino/ESP32 sketches in a **provisioning** style:

1. **Declare Runtime** — A single `Runtime rt;` instance acts as the orchestrator.
2. **Declare Modules (Components)** — Each component is an object (e.g. `LED`, `PushButton`, `DHTSensor`). Components own their internal pin configuration and non-blocking logic.
3. **Register Modules** — Call `rt.addModule(module);` for every component you want managed.
4. **Begin Runtime** — `rt.begin()` initializes all registered modules (pins, timers, initial states).
5. **Main Loop** — Call `rt.loopOnce()` on every loop iteration. This updates sensors, runs debouncers, processes scheduler tasks, and dispatches events.
6. **Your Logic** — After `rt.loopOnce()`, write rules using the high-level API:
   * `btn.pressed()`, `pir.motionStarted()`
   * `led.on()` / `led.off()` / `led.toggle()`
   * `buzzer.play(pattern)`
   * `dht.temperatureC()` / `dht.humidity()`

---

## 📁 Library File Structure

This repository is organized for **Arduino Library Manager** compatibility and clean component scaling:

```
ATGENXlib/
├─ src/
│  ├─ ATGENXlib.h                    ← Single public include
│  ├─ core/
│  │  ├─ ATG_Runtime.h / .cpp
│  │  ├─ ATG_Module.h
│  │  ├─ ATG_EventBus.h
│  │  ├─ ATG_Scheduler.h
│  │  ├─ ATG_Time.h
│  │  └─ ATG_Types.h
│  ├─ io/
│  │  ├─ ATG_DigitalInput.h
│  │  ├─ ATG_DigitalOutput.h
│  │  └─ ATG_PWMOutput.h
│  ├─ components/
│  │  ├─ sensors/
│  │  │  ├─ ATG_PushButton.h
│  │  │  ├─ ATG_PIRMotion.h
│  │  │  ├─ ATG_DHTSensor.h
│  │  │  ├─ ATG_FlameDigital.h
│  │  │  ├─ ATG_ReedSwitch.h
│  │  │  ├─ ATG_IrObstacle.h
│  │  │  └─ ATG_SoundDigital.h
│  │  ├─ actuators/
│  │  │  ├─ ATG_LED.h
│  │  │  ├─ ATG_Relay1Ch.h
│  │  │  └─ ATG_ActiveBuzzer.h
│  │  └─ common/
│  │     └─ ATG_ComponentBase.h
│  ├─ boards/
│  │  ├─ ATG_Board.h
│  │  ├─ ATG_Board_UNO.h
│  │  └─ ATG_Board_ESP32.h
│  └─ utils/
│     ├─ ATG_Log.h
│     └─ ATG_Assert.h
│
├─ examples/
│  ├─ 01_LED_Blink/
│  ├─ 02_PushButton_Toggle_LED/
│  ├─ 03_PIR_TurnOn_LED/
│  ├─ 04_DHT_Read/
│  ├─ 05_Relay_Control/
│  ├─ 06_Buzzer_Pattern/
│  ├─ 07_Flame_Digital/
│  ├─ 08_ReedSwitch_Alarm/
│  ├─ 09_IrObstacle_Digital/
│  └─ 10_SoundSensor_Digital/
│
├─ keywords.txt
├─ library.properties
├─ LICENSE
└─ README.md
```

> **Notes:**
>
> * `src/ATGENXlib.h` is the **single public include** that exports the core + all components.
> * Every component lives in `src/components/...` and must be **non-blocking**.
> * Examples are numbered to keep the Library Manager list clean and beginner-friendly.

---

## ✅ Naming Convention

To avoid ambiguous compilation errors, **never name a variable the same as its class**.

```cpp
// ✅ Correct
LED led(13);
PushButton btn(2);
DHTSensor dht(4, DHT22);

// ❌ Incorrect — will cause compilation errors
LED LED(13);
PushButton PushButton(2);
```

---

## 📄 License

See [LICENSE](https://claude.ai/chat/LICENSE) for details.
