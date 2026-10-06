#pragma once
// Not part of <ATGENXlib.h> (pulls in Wire):
//   #include <components/sensors/ATG_MPU6050.h>

#include <Arduino.h>
#include <Wire.h>
#include "../base/ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

/**
 * MPU-6050 accelerometer + gyroscope (I2C). Self-contained, no extra library.
 *
 *   MPU6050Sensor imu;               // address 0x68 (0x69 if AD0 is high)
 *   imu.tiltX(); imu.tiltY();        // degrees (roll / pitch) from gravity
 *   imu.accelX(); imu.gyroZ();       // g, °/s
 *   if (imu.shaken()) buzzer.beep();
 */
class MPU6050Sensor : public ComponentBase {
public:
  explicit MPU6050Sensor(uint8_t address = 0x68, uint16_t sampleMs = 20)
  : ComponentBase(0xFF), _addr(address), _sampleMs(sampleMs) {}

  const __FlashStringHelper* name() const override { return F("ATG_MPU6050"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    Wire.begin();
    const uint8_t who = read8(0x75);
    _ok = (who == 0x68 || who == 0x70 || who == 0x72); // MPU6050 / 6500 / 9250 family
    write8(0x6B, 0x00); // wake up
    write8(0x1C, 0x00); // ±2 g
    write8(0x1B, 0x00); // ±250 °/s
    _last = nowMs() - _sampleMs;
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (!_ok || !elapsed(_last, _sampleMs)) return;
    _last = nowMs();
    uint8_t d[14];
    Wire.beginTransmission(_addr);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)14);
    for (uint8_t i = 0; i < 14; i++) d[i] = Wire.available() ? Wire.read() : 0;
    const auto s16 = [&](uint8_t i) { return (int16_t)((d[i] << 8) | d[i + 1]); };
    _ax = s16(0) / 16384.0f;
    _ay = s16(2) / 16384.0f;
    _az = s16(4) / 16384.0f;
    _tempC = s16(6) / 340.0f + 36.53f;
    _gx = s16(8) / 131.0f;
    _gy = s16(10) / 131.0f;
    _gz = s16(12) / 131.0f;
  }

  float accelX() const { return _ax; }
  float accelY() const { return _ay; }
  float accelZ() const { return _az; }
  float gyroX() const { return _gx; }
  float gyroY() const { return _gy; }
  float gyroZ() const { return _gz; }
  float temperatureC() const { return _tempC; }

  /** Roll: rotation around X, from gravity (degrees, ±180). */
  float tiltX() const { return atan2(_ay, _az) * 57.29578f; }
  /** Pitch: rotation around Y, from gravity (degrees, ±90). */
  float tiltY() const { return atan2(-_ax, sqrt(_ay * _ay + _az * _az)) * 57.29578f; }
  /** Total acceleration in g (≈1 at rest). */
  float accelMagnitude() const { return sqrt(_ax * _ax + _ay * _ay + _az * _az); }
  /** Moving/shaken: total acceleration differs from 1 g by more than `threshold`. */
  bool shaken(float threshold = 0.5f) const { return fabs(accelMagnitude() - 1.0f) > threshold; }
  bool ok() const { return _ok; }

private:
  uint8_t read8(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
  }
  void write8(uint8_t reg, uint8_t v) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(v);
    Wire.endTransmission();
  }

  uint8_t _addr;
  uint16_t _sampleMs;
  ms_t _last = 0;
  bool _ok = false;
  float _ax = 0, _ay = 0, _az = 1, _gx = 0, _gy = 0, _gz = 0, _tempC = NAN;
};

} // namespace atg
