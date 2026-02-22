#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

class PushButton : public DigitalInput {
public:
  using ClickCallback = void(*)(uint8_t clicks);
  using VoidCallback  = void(*)();

  PushButton(
    uint8_t pin,
    bool pullup = true,
    uint16_t debounceMs = 35,
    uint16_t multiClickWindowMs = 380,  // نافذة تجميع الضغطات
    uint16_t longPressMs = 650          // زمن الضغط المطوّل
  )
  : DigitalInput(pin, pullup ? InputMode::Pullup : InputMode::Normal, debounceMs),
    _multiWin(multiClickWindowMs),
    _longMs(longPressMs) {}

  const __FlashStringHelper* name() const override { return F("ATG_Button"); }

  // لازم تتنادي في loop
  void update() {
    // DigitalInput غالبًا عنده update/tick في البايس
    // لو اسمها tick() عندك بدّل السطر ده
    DigitalInput::update();

    const uint32_t now = millis();

    // --- Detect press/release edges ---
    if (pressed()) {
      _pressing = true;
      _pressStart = now;
      _longFired = false;
      if (_onPress) _onPress();
    }

    if (released()) {
      _pressing = false;

      // لو long press اتنفّذ خلاص، ما نحسبش click
      if (!_longFired) {
        _clicks++;
        _lastRelease = now;
        _awaitFinalize = true;
      }

      if (_onRelease) _onRelease();
    }

    // --- Long press ---
    if (_pressing && !_longFired && (now - _pressStart >= _longMs)) {
      _longFired = true;
      _awaitFinalize = false;
      _clicks = 0;
      if (_onLongPress) _onLongPress();
    }

    // --- Finalize multi-click when window expires ---
    if (_awaitFinalize && _clicks > 0 && (now - _lastRelease >= _multiWin)) {
      const uint8_t c = _clicks;
      _clicks = 0;
      _awaitFinalize = false;

      // callback عام حسب عدد الضغطات
      if (_onClicks) _onClicks(c);

      // callbacks مخصوصة
      if (c == 1 && _onSingle) _onSingle();
      else if (c == 2 && _onDouble) _onDouble();
      else if (c == 3 && _onTriple) _onTriple();
    }
  }

  // pressed = Falling في حالة pullup
  bool pressed() const { return (edge() == Edge::Falling); }
  bool released() const { return (edge() == Edge::Rising); }
  bool isPressed() const { return isLow(); }

  // --- Setters للـ callbacks ---
  void onClicks(ClickCallback cb) { _onClicks = cb; }
  void onSingle(VoidCallback cb)  { _onSingle = cb; }
  void onDouble(VoidCallback cb)  { _onDouble = cb; }
  void onTriple(VoidCallback cb)  { _onTriple = cb; }
  void onLongPress(VoidCallback cb){ _onLongPress = cb; }
  void onPress(VoidCallback cb)   { _onPress = cb; }
  void onRelease(VoidCallback cb) { _onRelease = cb; }

  // تخصيص الإعدادات بعد الإنشاء
  void setMultiClickWindow(uint16_t ms) { _multiWin = ms; }
  void setLongPressMs(uint16_t ms)      { _longMs = ms; }

private:
  // timings
  uint16_t _multiWin;
  uint16_t _longMs;

  // state
  bool _pressing = false;
  bool _longFired = false;
  bool _awaitFinalize = false;

  uint8_t  _clicks = 0;
  uint32_t _pressStart = 0;
  uint32_t _lastRelease = 0;

  // callbacks
  ClickCallback _onClicks = nullptr;
  VoidCallback _onSingle = nullptr;
  VoidCallback _onDouble = nullptr;
  VoidCallback _onTriple = nullptr;
  VoidCallback _onLongPress = nullptr;
  VoidCallback _onPress = nullptr;
  VoidCallback _onRelease = nullptr;
};

} // namespace atg