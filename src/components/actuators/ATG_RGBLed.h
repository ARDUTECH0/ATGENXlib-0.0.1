#pragma once
#include <Arduino.h>
#include "../base/ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

/**
 * RGB LED on three PWM pins.
 *
 *   RGBLed mood(9, 10, 11);                 // R, G, B (common cathode)
 *   RGBLed mood(9, 10, 11, true);           // common anode
 *   mood.setColor(255, 80, 0);              // orange (and turns it on)
 *   mood.on(); mood.off(); mood.toggle();   // keeps the chosen color
 *   mood.setLevel(knob.ratio());            // brightness of the current color
 */
class RGBLed : public ComponentBase {
public:
  RGBLed(uint8_t rPin, uint8_t gPin, uint8_t bPin, bool commonAnode = false,
         uint8_t r = 255, uint8_t g = 255, uint8_t b = 255)
  : ComponentBase(rPin), _g(gPin), _b(bPin), _anode(commonAnode), _cr(r), _cg(g), _cb(b) {}

  const __FlashStringHelper* name() const override { return F("ATG_RGBLed"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    pinMode(_pin, OUTPUT);
    pinMode(_g, OUTPUT);
    pinMode(_b, OUTPUT);
    off();
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (_timerActive && elapsed(_timerStart, _timerMs)) {
      _timerActive = false;
      off();
    }
  }

  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    _cr = r; _cg = g; _cb = b;
    _on = true;
    apply();
  }

  void on()  { _on = true;  apply(); }
  void off() { _on = false; apply(); }
  void toggle() { _on ? off() : on(); }
  bool state() const { return _on; }

  void onFor(uint32_t ms) {
    on();
    _timerStart = nowMs();
    _timerMs = ms;
    _timerActive = true;
  }

  /** Brightness of the current color, 0.0 .. 1.0. */
  void setLevel(float ratio) {
    _timerActive = false;
    const float r = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);
    _brightness = (uint8_t)(r * 255.0f + 0.5f);
    _on = _brightness > 0;
    apply();
  }

private:
  void write(uint8_t pin, uint8_t v) { analogWrite(pin, _anode ? 255 - v : v); }
  uint8_t scale(uint8_t c) const { return (uint8_t)((uint16_t)c * _brightness / 255); }
  void apply() {
    write(_pin, _on ? scale(_cr) : 0);
    write(_g, _on ? scale(_cg) : 0);
    write(_b, _on ? scale(_cb) : 0);
  }

  uint8_t _g, _b;
  bool _anode;
  uint8_t _cr, _cg, _cb;
  uint8_t _brightness = 255;
  bool _on = false;
  bool _timerActive = false;
  ms_t _timerStart = 0;
  uint32_t _timerMs = 0;
};

} // namespace atg
