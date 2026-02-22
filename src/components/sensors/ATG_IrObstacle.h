#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

class IrObstacle : public DigitalInput {
public:
  IrObstacle(uint8_t pin, bool activeLow=true, uint16_t debounceMs=25)
  : DigitalInput(pin, InputMode::Normal, debounceMs), _activeLow(activeLow) {}

  const __FlashStringHelper* name() const override { return F("ATG_IrObstacle"); }

  bool detected() const {
    bool high = isHigh();
    return _activeLow ? !high : high;
  }

private:
  bool _activeLow;
};

} // namespace atg