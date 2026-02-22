#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

class FlameDigital : public DigitalInput {
public:
  FlameDigital(uint8_t pin, bool activeLow=true, uint16_t debounceMs=30)
  : DigitalInput(pin, InputMode::Normal, debounceMs), _activeLow(activeLow) {}

  const __FlashStringHelper* name() const override { return F("ATG_Flame"); }

  bool detected() const {
    bool high = isHigh();
    return _activeLow ? !high : high;
  }

  bool started() const {
    // لو activeLow: detection عند Falling
    if (_activeLow) return edge() == Edge::Falling;
    return edge() == Edge::Rising;
  }

private:
  bool _activeLow;
};

} // namespace atg