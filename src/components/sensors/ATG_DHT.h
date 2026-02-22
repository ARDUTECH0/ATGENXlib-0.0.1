#pragma once
#include <Arduino.h>
#include "../../core/ATG_Log.h"
#include "../../core/ATG_Time.h"
#include "../base/ATG_ComponentBase.h"

// External dependency
#include <DHT.h>

namespace atg {

enum class DhtType : uint8_t { DHT11=11, DHT22=22 };

class DHTSensor : public ComponentBase {
public:
  DHTSensor(uint8_t pin, DhtType type=DhtType::DHT11, uint16_t sampleMs=2000)
  : ComponentBase(pin), _type(type), _sampleMs(sampleMs), _dht(pin, (uint8_t)type) {}

  const __FlashStringHelper* name() const override { return F("ATG_DHT"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    _dht.begin();
    _last = 0;
    _tempC = NAN;
    _hum = NAN;
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (_sampleMs == 0) return;
    if (!elapsed(_last, _sampleMs)) return;
    _last = nowMs();

    float h = _dht.readHumidity();
    float t = _dht.readTemperature(); // Celsius

    if (!isnan(h)) _hum = h;
    if (!isnan(t)) _tempC = t;
  }

  float temperatureC() const { return _tempC; }
  float humidity() const { return _hum; }

private:
  DhtType _type;
  uint16_t _sampleMs;
  DHT _dht;

  ms_t _last{0};
  float _tempC;
  float _hum;
};

} // namespace atg