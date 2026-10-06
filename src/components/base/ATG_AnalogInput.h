#pragma once
#include <Arduino.h>
#include "ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

// Native ADC full scale per architecture. value() is always reported on the
// classic Arduino 0..1023 scale so sketches behave the same on every board.
#if defined(ESP32)
  #define ATG_ADC_MAX 4095
#else
  #define ATG_ADC_MAX 1023
#endif

/**
 * Smoothed, non-blocking analog reading (potentiometers, soil, gas, water,
 * rain and light sensors). Samples every `sampleMs` and applies a light
 * exponential moving average so values don't jitter.
 */
class AnalogInput : public ComponentBase {
public:
  explicit AnalogInput(uint8_t pin, uint16_t sampleMs = 20, uint8_t smoothing = 3)
  : ComponentBase(pin), _sampleMs(sampleMs), _shift(smoothing > 6 ? 6 : smoothing) {}

  Result begin(Runtime& rt) override {
    (void)rt;
    pinMode(_pin, INPUT);
    _acc = (uint32_t)readScaled() << _shift;
    _value = (uint16_t)(_acc >> _shift);
    _last = nowMs();
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (!elapsed(_last, _sampleMs)) return;
    _last = nowMs();
    // EMA with acc = value << shift:  acc += sample - acc / 2^shift
    _acc = _acc - (_acc >> _shift) + readScaled();
    _value = (uint16_t)(_acc >> _shift);
  }

  /** 0..1023 on every board. */
  uint16_t value() const { return _value; }
  /** 0.0 .. 1.0 */
  float ratio() const { return _value / 1023.0f; }
  /** 0 .. 100 */
  uint8_t percent() const { return (uint8_t)((_value * 100UL + 511) / 1023); }

protected:
  uint16_t readScaled() const {
    uint32_t raw = (uint32_t)analogRead(_pin);
    if (raw > ATG_ADC_MAX) raw = ATG_ADC_MAX;
    return (uint16_t)(raw * 1023UL / ATG_ADC_MAX);
  }

  uint16_t _sampleMs;
  uint8_t _shift;
  uint32_t _acc = 0;
  uint16_t _value = 0;
  ms_t _last = 0;
};

} // namespace atg
