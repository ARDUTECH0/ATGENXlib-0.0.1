#pragma once
#include <Arduino.h>
#include "../../core/ATG_Module.h"
#include "../../core/ATG_Types.h"

namespace atg {

class ComponentBase : public Module {
public:
  explicit ComponentBase(uint8_t pin) : _pin(pin) {}
  uint8_t pin() const { return _pin; }

protected:
  uint8_t _pin;
};

} // namespace atg