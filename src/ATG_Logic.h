#pragma once
/**
 * ATG_Logic — the parts the rules on the board read and switch, for the sketches the ATGenX Builder writes with
 * logic in them ("Temperature > 30 → Fan", "Door opens → message"). Small, no runtime, no cloud: each part is
 * begun once and polled from loop().
 *
 *   ATG_In        a switch or a digital sensor, debounced (active LOW or HIGH)
 *   ATG_Out       an output switched on and off (an LED, a relay, a buzzer)
 *   ATG_Sonar     an HC-SR04 measured ten times a second
 *   ATG_DhtReader a DHT11 / DHT22 read every 2.5 s, its last good reading kept
 *   atgxScale     an analog reading as 0…1023 on every board (the ESP32 reads 0…4095)
 *   atgxPercent   0…1023 as 0…100
 *   atgxNum       a reading as text for a message: "25.1C", "-" when there's none
 */

#include <Arduino.h>
#include <math.h>
#include <DHT.h>

/** A switch or sensor read for the logic on the board: debounced 30 ms, active LOW or HIGH. */
class ATG_In {
 public:
  ATG_In(uint8_t pin, bool activeLow) : _pin(pin), _low(activeLow) {}
  void begin() { pinMode(_pin, _low ? INPUT_PULLUP : INPUT); }
  bool isActive() {
    const bool r = (digitalRead(_pin) == LOW) == _low;
    if (r != _seen) {
      _seen = r;
      _at = millis();
    }
    if (millis() - _at >= 30) _state = _seen;
    return _state;
  }

 private:
  uint8_t _pin;
  bool _low;
  bool _state = false, _seen = false;
  uint32_t _at = 0;
};

/** An output switched by the logic on the board. */
class ATG_Out {
 public:
  ATG_Out(uint8_t pin, bool activeLow) : _pin(pin), _low(activeLow) {}
  void begin() {
    pinMode(_pin, OUTPUT);
    write(false);
  }
  void write(bool v) {
    on = v;
    digitalWrite(_pin, v != _low ? HIGH : LOW);
  }
  void turnOn() { write(true); }
  void turnOff() { write(false); }
  void toggle() { write(!on); }
  bool isOn() const { return on; }
  bool on = false;

 private:
  uint8_t _pin;
  bool _low;
};

/** An HC-SR04 measured for the logic on the board, ten times a second. */
class ATG_Sonar {
 public:
  ATG_Sonar(uint8_t trig, uint8_t echo) : _trig(trig), _echo(echo) {}
  void begin() {
    pinMode(_trig, OUTPUT);
    pinMode(_echo, INPUT);
  }
  void poll() {
    if (millis() - _at < 100) return;
    _at = millis();
    digitalWrite(_trig, LOW);
    delayMicroseconds(2);
    digitalWrite(_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trig, LOW);
    const unsigned long us = pulseIn(_echo, HIGH, 24000UL);
    ok = us > 0;
    cm = ok ? us / 58.0f : 0;
  }
  float cm = 0;
  bool ok = false;

 private:
  uint8_t _trig, _echo;
  uint32_t _at = 0;
};

/** A DHT11 / DHT22 read every 2.5 s for the logic on the board; a failed read keeps the last good one. */
class ATG_DhtReader {
 public:
  ATG_DhtReader(uint8_t pin, uint8_t type = DHT22) : _dht(pin, type) {}
  void begin() { _dht.begin(); }
  void poll() {
    if (_at && millis() - _at < 2500) return;
    _at = millis() | 1;
    const float t = _dht.readTemperature(), h = _dht.readHumidity();
    if (!isnan(t)) _t = t;
    if (!isnan(h)) _h = h;
  }
  float temperatureC() const { return _t; }
  float humidity() const { return _h; }

 private:
  DHT _dht;
  float _t = NAN, _h = NAN;
  uint32_t _at = 0;
};

/** An analog reading as 0 to 1023 on every board, the scale the Builder's numbers use (the ESP32 reads 0 to 4095). */
inline int atgxScale(int raw) {
  if (raw < 0) return 0;
#if defined(ESP32)
  return raw >> 2;
#else
  return raw;
#endif
}

/** 0…1023 as 0…100 */
inline int atgxPercent(int v) { return (int)((v * 100L + 511) / 1023); }

/** A reading as text for a message: "25.1C" — "-" when the sensor has none yet. */
inline String atgxNum(float v, uint8_t decimals, const char* unit) {
  if (isnan(v)) return String("-");
  String s(v, (unsigned int)decimals);
  s += unit;
  return s;
}
