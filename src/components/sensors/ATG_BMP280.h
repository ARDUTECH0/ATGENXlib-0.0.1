#pragma once
// Not part of <ATGENXlib.h> (pulls in Wire):
//   #include <components/sensors/ATG_BMP280.h>

#include <Arduino.h>
#include <Wire.h>
#include "../base/ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

/**
 * BMP280 temperature & pressure sensor (I2C). Self-contained — no extra
 * library — using Bosch's integer compensation formulas.
 *
 *   BMP280Sensor air;            // address 0x76 (0x77 if SDO is high)
 *   air.temperatureC();  air.pressureHpa();  air.altitudeM();  air.ok();
 */
class BMP280Sensor : public ComponentBase {
public:
  explicit BMP280Sensor(uint8_t address = 0x76, uint16_t sampleMs = 1000)
  : ComponentBase(0xFF), _addr(address), _sampleMs(sampleMs) {}

  const __FlashStringHelper* name() const override { return F("ATG_BMP280"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    Wire.begin();
    const uint8_t id = read8(0xD0);
    _ok = (id == 0x58 || id == 0x60); // BMP280, or a BME280
    if (!_ok) return Result::Ok;      // keep running; ok() reports the problem

    uint8_t c[24];
    readBytes(0x88, c, sizeof(c));
    _T1 = (uint16_t)(c[1] << 8 | c[0]);
    _T2 = (int16_t)(c[3] << 8 | c[2]);
    _T3 = (int16_t)(c[5] << 8 | c[4]);
    _P1 = (uint16_t)(c[7] << 8 | c[6]);
    for (uint8_t i = 0; i < 8; i++) _dig[i] = (int16_t)(c[9 + i * 2] << 8 | c[8 + i * 2]);

    write8(0xF5, 0xA0);  // standby 1 s, filter off
    write8(0xF4, 0x27);  // temp ×1, pressure ×1, normal mode
    _last = nowMs() - _sampleMs;
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (!_ok || !elapsed(_last, _sampleMs)) return;
    _last = nowMs();
    uint8_t d[6];
    readBytes(0xF7, d, 6);
    const int32_t adcP = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    const int32_t adcT = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);

    // Bosch BMP280 datasheet, 8.2 (integer versions)
    int32_t v1 = ((((adcT >> 3) - ((int32_t)_T1 << 1))) * (int32_t)_T2) >> 11;
    int32_t v2 = (((((adcT >> 4) - (int32_t)_T1) * ((adcT >> 4) - (int32_t)_T1)) >> 12) * (int32_t)_T3) >> 14;
    const int32_t tFine = v1 + v2;
    _tempC = ((tFine * 5 + 128) >> 8) / 100.0f;

    int64_t p1 = (int64_t)tFine - 128000;
    int64_t p2 = p1 * p1 * (int64_t)_dig[4];
    p2 = p2 + ((p1 * (int64_t)_dig[3]) << 17);
    p2 = p2 + (((int64_t)_dig[2]) << 35);
    p1 = ((p1 * p1 * (int64_t)_dig[1]) >> 8) + ((p1 * (int64_t)_dig[0]) << 12);
    p1 = (((((int64_t)1) << 47) + p1)) * (int64_t)_P1 >> 33;
    if (p1 == 0) return;
    int64_t p = 1048576 - adcP;
    p = (((p << 31) - p2) * 3125) / p1;
    p1 = (((int64_t)_dig[7]) * (p >> 13) * (p >> 13)) >> 25;
    p2 = (((int64_t)_dig[6]) * p) >> 19;
    p = ((p + p1 + p2) >> 8) + (((int64_t)_dig[5]) << 4);
    _pressureHpa = (p / 256.0f) / 100.0f;
  }

  float temperatureC() const { return _tempC; }
  float pressureHpa() const { return _pressureHpa; }
  /** Altitude estimate from pressure (standard atmosphere, sea level 1013.25 hPa). */
  float altitudeM(float seaLevelHpa = 1013.25f) const {
    return _pressureHpa > 0 ? 44330.0f * (1.0f - pow(_pressureHpa / seaLevelHpa, 0.1903f)) : 0;
  }
  /** False if no BMP280 answered at the address. */
  bool ok() const { return _ok; }

private:
  uint8_t read8(uint8_t reg) {
    uint8_t v = 0;
    readBytes(reg, &v, 1);
    return v;
  }
  void write8(uint8_t reg, uint8_t v) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(v);
    Wire.endTransmission();
  }
  void readBytes(uint8_t reg, uint8_t* buf, uint8_t n) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, n);
    for (uint8_t i = 0; i < n; i++) buf[i] = Wire.available() ? Wire.read() : 0;
  }

  uint8_t _addr;
  uint16_t _sampleMs;
  ms_t _last = 0;
  bool _ok = false;
  uint16_t _T1 = 0, _P1 = 0;
  int16_t _T2 = 0, _T3 = 0, _dig[8] = {0};
  float _tempC = NAN, _pressureHpa = NAN;
};

} // namespace atg
