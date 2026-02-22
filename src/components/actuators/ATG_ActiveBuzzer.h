#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalOutput.h"
#include "../../core/ATG_Time.h"

namespace atg {

class ActiveBuzzer : public DigitalOutput {
public:
  ActiveBuzzer(uint8_t pin, bool activeLow=false) : DigitalOutput(pin, activeLow) {}

  const __FlashStringHelper* name() const override { return F("ATG_Buzzer"); }

  // Beep non-blocking
  void beep(uint16_t onMs=100, uint16_t offMs=100, uint8_t times=1) {
    _onMs = onMs; _offMs = offMs; _times = times;
    _phase = 0; // 0=on,1=off
    _countDone = 0;
    _running = true;
    _last = nowMs();
    on();
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (!_running) return;

    if (_phase == 0) { // ON
      if (elapsed(_last, _onMs)) {
        off();
        _phase = 1;
        _last = nowMs();
      }
    } else { // OFF
      if (elapsed(_last, _offMs)) {
        _countDone++;
        if (_countDone >= _times) {
          _running = false;
          off();
          return;
        }
        on();
        _phase = 0;
        _last = nowMs();
      }
    }
  }

private:
  bool _running{false};
  uint16_t _onMs{100}, _offMs{100};
  uint8_t _times{1};
  uint8_t _phase{0};
  uint8_t _countDone{0};
  ms_t _last{0};
};

} // namespace atg