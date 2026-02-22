#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

class PushButton : public DigitalInput {
public:
  // غالبًا بنستخدم Pullup للزرار
  PushButton(uint8_t pin, bool pullup=true, uint16_t debounceMs=35)
  : DigitalInput(pin, pullup?InputMode::Pullup:InputMode::Normal, debounceMs) {}

  const __FlashStringHelper* name() const override { return F("ATG_Button"); }

  // pressed = انتقال من HIGH->LOW في حالة pullup
  bool pressed() const {
    // Pullup: stable LOW يعني مضغوط
    return (edge() == Edge::Falling);
  }

  bool released() const { return (edge() == Edge::Rising); }

  bool isPressed() const { return isLow(); }
};

} // namespace atg