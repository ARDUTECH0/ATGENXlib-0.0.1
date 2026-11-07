#pragma once
#include <Arduino.h>
#include <Wire.h>

namespace atg {

/**
 * The MPU6050 as a tilt meter for a robot: the angle it leans at and how fast it's falling, 200 times a second. The gyro
 * is quick but drifts and the accelerometer is steady but noisy, so the angle uses both.
 *
 *   TiltSensor tilt;          // 0x68; tilt(0.999, 0x69) with AD0 high
 *   tilt.begin();             // keep the robot still: it learns the gyro's zero
 *   if (tilt.update()) Serial.println(tilt.angle());
 *
 * Fix it firmly near the wheel axle: any wobble of the sensor reads as the robot falling. If leaning forward reads negative,
 * turn the module round.
 */
class TiltSensor {
public:
  /** `blend`: the gyro's share of the angle. High on purpose: the accelerometer also feels the robot speeding up. */
  explicit TiltSensor(float blend = 0.999f, uint8_t address = 0x68) : _addr(address) { setBlend(blend); }

  void setBlend(float blend) { _blend = constrain(blend, 0.5f, 0.9999f); }

  void begin() {
    Wire.begin();
    Wire.setClock(400000);
    Wire.beginTransmission(_addr);
    Wire.write(0x6B);
    Wire.write(0);  // wake it
    Wire.endTransmission();
    delay(100);
    // the robot is still at power-on: that's the moment to learn what the gyro reads at rest. Every MPU6050 has its own.
    long total = 0;
    for (int i = 0; i < 200; i++) {
      Wire.beginTransmission(_addr);
      Wire.write(0x45);
      Wire.endTransmission(false);
      Wire.requestFrom(_addr, (uint8_t)2);
      total += word16();
      delay(2);
    }
    _zero = total / 200.0f;
    _t = micros();
  }

  /** Call on every pass of loop(). True when a new reading came in (200 times a second). */
  bool update() {
    if (micros() - _t < 5000) return false;
    const float dt = (micros() - _t) / 1000000.0f;
    _t = micros();
    Wire.beginTransmission(_addr);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)14);
    if (Wire.available() < 14) return false;
    const int16_t ax = word16();
    word16();  // Y
    const int16_t az = word16();
    word16();  // temperature
    word16();  // gyro X
    const int16_t gy = word16();
    word16();  // gyro Z
    _rate = (gy - _zero) / 131.0f;
    // leaning towards the module's +X, gravity reads negative on X while the gyro's Y reads positive: the minus makes both
    // say the same thing, or the filter would pull the angle the wrong way and the robot would fall
    const float seen = atan2(-(float)ax, (float)az) * 57.2958f;
    // the first reading sets the angle: the filter is slow on purpose, and would take seconds to catch up from 0
    if (!_started) {
      _angle = seen;
      _started = true;
    } else {
      _angle = _blend * (_angle + _rate * dt) + (1 - _blend) * seen;
    }
    return true;
  }

  /** Degrees; positive leaning towards the module's +X. */
  float angle() const { return _angle; }
  /** Degrees a second. */
  float rate() const { return _rate; }

private:
  /** the high byte first: written as one expression, C++ doesn't promise which read runs first */
  static int16_t word16() {
    const uint8_t hi = Wire.read();
    const uint8_t lo = Wire.read();
    return (int16_t)((hi << 8) | lo);
  }

  uint8_t _addr;
  float _blend = 0.999f;
  float _zero = 0;
  float _angle = 0;
  float _rate = 0;
  bool _started = false;
  uint32_t _t = 0;
};

} // namespace atg
