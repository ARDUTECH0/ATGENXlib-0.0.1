#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

class ReedSwitch : public DigitalInput {
public:
  ReedSwitch(uint8_t pin, bool pullup=true, uint16_t debounceMs=20)
  : DigitalInput(pin, pullup?InputMode::Pullup:InputMode::Normal, debounceMs), _pullup(pullup) {}

  const __FlashStringHelper* name() const override { return F("ATG_Reed"); }

  bool closed() const {
    // Pullup: closed => LOW
    return _pullup ? isLow() : isHigh();
  }

private:
  bool _pullup;
};

} // namespace atg