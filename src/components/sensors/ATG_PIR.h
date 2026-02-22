#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

class PIRMotion : public DigitalInput {
public:
  PIRMotion(uint8_t pin, uint16_t debounceMs=50)
  : DigitalInput(pin, InputMode::Normal, debounceMs) {}

  const __FlashStringHelper* name() const override { return F("ATG_PIR"); }

  bool motion() const { return isHigh(); }
  bool motionStarted() const { return edge() == Edge::Rising; }
  bool motionEnded() const { return edge() == Edge::Falling; }
};

} // namespace atg