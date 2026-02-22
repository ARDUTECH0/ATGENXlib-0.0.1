#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalOutput.h"
#include "../../core/ATG_Types.h"

namespace atg {

class LED : public DigitalOutput {
public:
  explicit LED(uint8_t pin, bool activeLow=false) : DigitalOutput(pin, activeLow) {}
  const __FlashStringHelper* name() const override { return F("ATG_LED"); }

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