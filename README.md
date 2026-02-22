# ATGENXlib

ATGENXlib is the official core library for the ATGenX visual Arduino/ESP32 builder platform.

It provides a modular runtime engine, component abstraction layer, and production-ready sensor/actuator modules designed for code generation systems.

---

## ✨ Features

- Modular Runtime Engine
- Event Bus System
- Non-blocking Scheduler
- State Machine Engine
- Board Abstraction (UNO / ESP32)
- Digital Input with Debounce & Edge Detection
- Digital Output with Active-LOW support
- Non-blocking Buzzer Patterns
- DHT Temperature & Humidity Support
- Production-ready Component Structure

---

## 🧱 Supported Boards

- Arduino UNO R3
- ESP32 DevKit V1
- ESP32 D4 Pico

---

## 🔌 Supported Components

### Sensors

- DHT11 / DHT22
- Push Button (Pullup supported)
- PIR Motion Sensor
- Flame Sensor (Digital)
- Reed Switch
- IR Obstacle Sensor
- Sound Sensor (Digital)

### Actuators

- LED (PWM supported)
- Relay 1-Channel
- Active Buzzer (non-blocking patterns)

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
