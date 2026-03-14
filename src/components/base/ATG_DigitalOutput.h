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

  void tick(Runtime& rt) override {
    (void)rt;

    if (_timerActive) {
      if (millis() - _timerStart >= _timerDuration) {
        write(_timerTarget);
        _timerActive = false;
      }
    }
  }

  // ─────────────────────────────
  // BASIC CONTROL
  // ─────────────────────────────

  void on()  { write(true);  }
  void off() { write(false); }

  void toggle() { write(!_state); }

  bool state() const { return _state; }

  // ─────────────────────────────
  // TIMED ACTIONS
  // ─────────────────────────────

  void onFor(uint32_t ms) {
    write(true);
    startTimer(false, ms);
  }

  void offFor(uint32_t ms) {
    write(false);
    startTimer(true, ms);
  }

  void pulse(uint32_t ms) {
    toggle();
    startTimer(!_state, ms);
  }

protected:

  void write(bool on) {
    _state = on;

    uint8_t level = on ? HIGH : LOW;
    if (_activeLow) level = (level==HIGH ? LOW : HIGH);

    digitalWrite(_pin, level);
  }

  void startTimer(bool targetState, uint32_t duration) {
    _timerTarget = targetState;
    _timerDuration = duration;
    _timerStart = millis();
    _timerActive = true;
  }

  bool _activeLow{false};
  bool _state{false};

  // timer system
  bool _timerActive{false};
  bool _timerTarget{false};
  uint32_t _timerStart{0};
  uint32_t _timerDuration{0};
};

} // namespace atg