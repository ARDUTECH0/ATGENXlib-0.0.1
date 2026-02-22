#pragma once
#include <Arduino.h>
#include "ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

enum class InputMode : uint8_t { Normal = 0, Pullup = 1 };
enum class Edge : uint8_t { None=0, Rising=1, Falling=2 };

class DigitalInput : public ComponentBase {
public:
  DigitalInput(uint8_t pin, InputMode mode=InputMode::Normal, uint16_t debounceMs=30)
  : ComponentBase(pin), _mode(mode), _debounce(debounceMs) {}

  // begin(): config pin + read initial state
  Result begin(Runtime& rt) override {
    (void)rt;
    pinMode(_pin, _mode==InputMode::Pullup ? INPUT_PULLUP : INPUT);
    _raw = digitalRead(_pin);
    _stable = _raw;
    _lastStable = _stable;
    _lastChange = nowMs();
    _edge = Edge::None;
    return Result::Ok;
  }

  // tick(): debounce + edge detect
  void tick(Runtime& rt) override {
    (void)rt;
    const uint8_t r = digitalRead(_pin);
    if (r != _raw) {
      _raw = r;
      _lastChange = nowMs();
    }

    if (elapsed(_lastChange, _debounce) && _stable != _raw) {
      _lastStable = _stable;
      _stable = _raw;
      if (_lastStable==LOW && _stable==HIGH) _edge = Edge::Rising;
      else if (_lastStable==HIGH && _stable==LOW) _edge = Edge::Falling;
      else _edge = Edge::None;
    } else {
      _edge = Edge::None;
    }
  }

  uint8_t raw() const { return _raw; }
  uint8_t stable() const { return _stable; }
  bool isHigh() const { return _stable == HIGH; }
  bool isLow()  const { return _stable == LOW;  }
  Edge edge() const { return _edge; }

protected:
  InputMode _mode;
  uint16_t _debounce;
  uint8_t _raw{LOW};
  uint8_t _stable{LOW};
  uint8_t _lastStable{LOW};
  ms_t _lastChange{0};
  Edge _edge{Edge::None};
};

} // namespace atg