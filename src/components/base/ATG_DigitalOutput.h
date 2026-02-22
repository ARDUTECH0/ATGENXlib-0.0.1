#pragma once
#include <Arduino.h>
#include "ATG_ComponentBase.h"

namespace atg {

class DigitalOutput : public ComponentBase {
public:
  explicit DigitalOutput(uint8_t pin, bool activeLow=false)
  : ComponentBase(pin), _activeLow(activeLow) {}

  Result begin(Runtime& rt) override {
    (void)rt;
    pinMode(_pin, OUTPUT);
    off();
    return Result::Ok;
  }

  void tick(Runtime& rt) override { (void)rt; /* no-op by default */ }

  void on()  { write(true);  }
  void off() { write(false); }
  void toggle() { write(!_state); }
  bool state() const { return _state; }

protected:
  void write(bool on) {
    _state = on;
    uint8_t level = on ? HIGH : LOW;
    if (_activeLow) level = (level==HIGH ? LOW : HIGH);
    digitalWrite(_pin, level);
  }

  bool _activeLow{false};
  bool _state{false};
};

} // namespace atg