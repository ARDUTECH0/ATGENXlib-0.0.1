#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

class SoundDigital : public DigitalInput {
public:
  SoundDigital(uint8_t pin, bool activeHigh=true, uint16_t debounceMs=20)
  : DigitalInput(pin, InputMode::Normal, debounceMs), _activeHigh(activeHigh) {}

  const __FlashStringHelper* name() const override { return F("ATG_SoundD"); }

  bool triggered() const {
    return _activeHigh ? isHigh() : isLow();
  }

  bool pulse() const {
    return _activeHigh ? (edge()==Edge::Rising) : (edge()==Edge::Falling);
  }

private:
  bool _activeHigh;
};

} // namespace atg