#pragma once
#include <Arduino.h>
#include "../base/ATG_ComponentBase.h"

namespace atg {

/**
 * Non-blocking 4-wire stepper (28BYJ-48 + ULN2003 by default). Unlike the
 * Arduino Stepper library it never blocks loop(): one step per tick when due.
 *
 *   StepperMotor arm(8, 9, 10, 11);          // IN1..IN4, 2048 steps/rev, 10 rpm
 *   arm.on();                  // rotate continuously   (off() stops & releases coils)
 *   arm.moveTo(512);           // go to an absolute position (steps)
 *   arm.setLevel(knob.ratio()); // follow a knob: 0..1 → 0..one revolution
 */
class StepperMotor : public ComponentBase {
public:
  StepperMotor(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4,
               uint16_t stepsPerRev = 2048, uint8_t rpm = 10)
  : ComponentBase(in1), _stepsPerRev(stepsPerRev) {
    _pins[0] = in1; _pins[1] = in2; _pins[2] = in3; _pins[3] = in4;
    setSpeed(rpm);
  }

  const __FlashStringHelper* name() const override { return F("ATG_Stepper"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    for (uint8_t i = 0; i < 4; i++) {
      pinMode(_pins[i], OUTPUT);
      digitalWrite(_pins[i], LOW);
    }
    _lastStepUs = micros();
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (_timerActive && (millis() - _timerStart) >= _timerMs) {
      _timerActive = false;
      off();
    }
    int8_t dir = 0;
    if (_continuous) dir = _reverse ? -1 : 1;
    else if (_position != _target) dir = _target > _position ? 1 : -1;
    if (dir == 0) return;

    const uint32_t now = micros();
    if ((uint32_t)(now - _lastStepUs) < _stepUs) return;
    _lastStepUs = now;
    _position += dir;
    energize((uint8_t)(((_position % 4) + 4) % 4));
    if (!_continuous && _position == _target) release();
  }

  void setSpeed(uint8_t rpm) {
    if (rpm == 0) rpm = 1;
    _stepUs = 60000000UL / ((uint32_t)_stepsPerRev * rpm);
  }

  /** Rotate continuously until off(). */
  void on() { _continuous = true; }
  /** Stop and de-energize the coils. */
  void off() { _continuous = false; _target = _position; release(); }
  void toggle() { _continuous ? off() : on(); }
  bool state() const { return _continuous || _position != _target; }

  void onFor(uint32_t ms) {
    on();
    _timerStart = millis();
    _timerMs = ms;
    _timerActive = true;
  }

  void forward() { _reverse = false; }
  void reverse() { _reverse = true; }

  void moveTo(long position) { _continuous = false; _target = position; }
  void move(long steps) { moveTo(_target + steps); }
  long position() const { return _position; }

  /** 0.0 .. 1.0 → 0 .. one full revolution (absolute). */
  void setLevel(float ratio) {
    const float r = ratio < 0 ? 0 : (ratio > 1 ? 1 : ratio);
    moveTo((long)(r * _stepsPerRev + 0.5f));
  }

private:
  // full-step, two coils on (strong torque)
  void energize(uint8_t phase) {
    static const uint8_t SEQ[4] = {0b1100, 0b0110, 0b0011, 0b1001};
    for (uint8_t i = 0; i < 4; i++) digitalWrite(_pins[i], (SEQ[phase] >> (3 - i)) & 1);
  }
  void release() {
    for (uint8_t i = 0; i < 4; i++) digitalWrite(_pins[i], LOW);
  }

  uint8_t _pins[4];
  uint16_t _stepsPerRev;
  uint32_t _stepUs = 2930;
  uint32_t _lastStepUs = 0;
  long _position = 0;
  long _target = 0;
  bool _continuous = false;
  bool _reverse = false;
  bool _timerActive = false;
  uint32_t _timerStart = 0;
  uint32_t _timerMs = 0;
};

} // namespace atg
