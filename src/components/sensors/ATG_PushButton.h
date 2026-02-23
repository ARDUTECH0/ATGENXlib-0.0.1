#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"
#include "../../core/ATG_Time.h"

namespace atg {

class PushButton : public DigitalInput {
public:
  using ClickCallback = void(*)(uint8_t clicks);
  using VoidCallback  = void(*)();

  static constexpr uint8_t MAX_BINDINGS = 12;

  // pullup افتراضي + debounce + window للضغطات + long press
  PushButton(
    uint8_t pin,
    bool pullup = true,
    uint16_t debounceMs = 35,
    uint16_t multiClickWindowMs = 450,
    uint16_t longPressMs = 650
  )
  : DigitalInput(pin, pullup ? InputMode::Pullup : InputMode::Normal, debounceMs),
    _multiWin(multiClickWindowMs),
    _longMs(longPressMs) {}

  const __FlashStringHelper* name() const override { return F("ATG_Button"); }

  // ✅ Runtime هو اللي هينادي tick() عبر rt.loopOnce()
  void tick(Runtime& rt) override {
    DigitalInput::tick(rt);

    // Press edge
    if (pressed()) {
      _pressing = true;
      _pressStart = nowMs();
      _longFired = false;
      if (_onPress) _onPress();
    }

    // Release edge
    if (released()) {
      _pressing = false;

      // لو long اتنفّذ، ما نحسبش click
      if (!_longFired) {
        _clicks++;
        _lastRelease = nowMs();
        _awaitFinalize = true;
      }

      if (_onRelease) _onRelease();
    }

    // Long press
    if (_pressing && !_longFired && elapsed(_pressStart, _longMs)) {
      _longFired = true;
      _awaitFinalize = false;
      _clicks = 0;
      if (_onLongPress) _onLongPress();
    }

    // Finalize multi-click after window
    if (_awaitFinalize && _clicks > 0 && elapsed(_lastRelease, _multiWin)) {
      const uint8_t c = _clicks;
      _clicks = 0;
      _awaitFinalize = false;

      if (_onClicks) _onClicks(c);

      // ✅ Bind لأي رقم ضغطات (5/10/...)
      for (uint8_t i = 0; i < _bindCount; i++) {
        if (_bindClicks[i] == c && _bindFns[i]) {
          _bindFns[i]();
          break;
        }
      }
    }
  }

  // pressed/released
  bool pressed()  const { return edge() == Edge::Falling; } // pullup
  bool released() const { return edge() == Edge::Rising;  }
  bool isPressed() const { return isLow(); }

  // إعدادات
  void setMultiClickWindow(uint16_t ms) { _multiWin = ms; }
  void setLongPressMs(uint16_t ms)      { _longMs = ms; }

  // callbacks (اختياري)
  void onClicks(ClickCallback cb) { _onClicks = cb; }
  void onLongPress(VoidCallback cb) { _onLongPress = cb; }
  void onPress(VoidCallback cb) { _onPress = cb; }
  void onRelease(VoidCallback cb) { _onRelease = cb; }

  // ✅ اربط “رقم الضغطات” بفنكشن
  bool bind(uint8_t clicks, VoidCallback cb) {
    if (!cb || clicks == 0) return false;

    // update existing
    for (uint8_t i = 0; i < _bindCount; i++) {
      if (_bindClicks[i] == clicks) { _bindFns[i] = cb; return true; }
    }

    // add new
    if (_bindCount >= MAX_BINDINGS) return false;
    _bindClicks[_bindCount] = clicks;
    _bindFns[_bindCount] = cb;
    _bindCount++;
    return true;
  }

private:
  uint16_t _multiWin;
  uint16_t _longMs;

  bool _pressing = false;
  bool _longFired = false;
  bool _awaitFinalize = false;

  uint8_t _clicks = 0;
  ms_t _pressStart = 0;
  ms_t _lastRelease = 0;

  ClickCallback _onClicks = nullptr;
  VoidCallback _onLongPress = nullptr;
  VoidCallback _onPress = nullptr;
  VoidCallback _onRelease = nullptr;

  uint8_t _bindCount = 0;
  uint8_t _bindClicks[MAX_BINDINGS] = {0};
  VoidCallback _bindFns[MAX_BINDINGS] = {nullptr};
};

} // namespace atg