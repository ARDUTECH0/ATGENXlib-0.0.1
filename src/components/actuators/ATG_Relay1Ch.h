#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalOutput.h"

namespace atg {

class Relay1Ch : public DigitalOutput {
public:
  Relay1Ch(uint8_t pin, bool activeLow=true) : DigitalOutput(pin, activeLow) {}
  const __FlashStringHelper* name() const override { return F("ATG_Relay1Ch"); }
};

} // namespace atg