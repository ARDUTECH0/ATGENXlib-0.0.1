#pragma once
#include <Arduino.h>
#include "../base/ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

/**
 * DC motor through an H-bridge (L298N, L9110, TB6612…).
 *
 *   DCMotor fan(9, 8);          // EN (PWM) pin, IN1 pin  — IN2 tied to GND
 *   DCMotor car(9, 8, 7);       // EN, IN1, IN2 — direction control
 *   fan.on(); fan.setSpeed(150); fan.setLevel(knob.ratio()); car.reverse();
 *
 * Without an EN pin (jumper on the board) pass ATG_NO_PIN as `enPin`; speed
 * then is all-or-nothing.
 */
#ifndef ATG_NO_PIN
#define ATG_NO_PIN 255
#endif

class DCMotor : public ComponentBase {
public:
  DCMotor(uint8_t enPin, uint8_t in1Pin, uint8_t in2Pin = ATG_NO_PIN, uint8_t speed = 255)
  : ComponentBase(enPin), _in1(in1Pin), _in2(in2Pin), _speed(speed) {}

  const __FlashStringHelper* name() const override { return F("ATG_DCMotor"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    if (_pin != ATG_NO_PIN) pinMode(_pin, OUTPUT);
    pinMode(_in1, OUTPUT);
    if (_in2 != ATG_NO_PIN) pinMode(_in2, OUTPUT);
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

  void on()  { _running = true;  apply(); }
  void off() { _running = false; apply(); }
  void toggle() { _running ? off() : on(); }
  bool state() const { return _running; }

  void onFor(uint32_t ms) {
    on();
    _timerStart = nowMs();
    _timerMs = ms;
    _timerActive = true;
  }

  /** 0..255; takes effect immediately if running. */
  void setSpeed(uint8_t speed) { _speed = speed; apply(); }
  uint8_t speed() const { return _speed; }

  /** 0.0 (stopped) .. 1.0 (full speed). */
  void setLevel(float ratio) {
    _timerActive = false;
    const float r = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);
    _speed = (uint8_t)(r * 255.0f + 0.5f);
    _running = _speed > 0;
    apply();
  }

  void forward() { _reverse = false; apply(); }
  void reverse() { _reverse = true; apply(); }
  bool isReversed() const { return _reverse; }

private:
  void apply() {
    const bool run = _running && _speed > 0;
    if (_in2 == ATG_NO_PIN) {
      // single direction input: IN1 HIGH = run (IN2 tied low on the module)
      digitalWrite(_in1, run ? HIGH : LOW);
    } else {
      digitalWrite(_in1, run && !_reverse ? HIGH : LOW);
      digitalWrite(_in2, run && _reverse ? HIGH : LOW);
    }
    if (_pin != ATG_NO_PIN) analogWrite(_pin, run ? _speed : 0);
  }

  uint8_t _in1, _in2;
  uint8_t _speed;
  bool _running = false;
  bool _reverse = false;
  bool _timerActive = false;
  ms_t _timerStart = 0;
  uint32_t _timerMs = 0;
};

} // namespace atg
