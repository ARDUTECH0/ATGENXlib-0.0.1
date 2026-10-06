#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalOutput.h"
#include "../../core/ATG_Types.h"

namespace atg {

class LED : public DigitalOutput {
public:
  explicit LED(uint8_t pin, bool activeLow=false) : DigitalOutput(pin, activeLow) {}
  const __FlashStringHelper* name() const override { return F("ATG_LED"); }

  /** 0.0 (off) .. 1.0 (full) — PWM, respects activeLow. */
  void setLevel(float ratio) {
    const float r = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);
    uint8_t v = (uint8_t)(r * 255.0f + 0.5f);
    _state = v > 0;
    analogWrite(_pin, _activeLow ? 255 - v : v);
  }

  // 0..255
  void setBrightness(uint8_t v) {
#if defined(ESP32)
    // ESP32: analogWrite موجود في Arduino core (في الغالب) لكن ممكن يتطلب ledc.
    // عشان الإنتاج: هنستخدم analogWrite مباشرة هنا كبداية.
    analogWrite(_pin, v);
#else
    analogWrite(_pin, v);
#endif
  }
};

} // namespace atg