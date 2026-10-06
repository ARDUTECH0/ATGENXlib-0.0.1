#pragma once
// Not part of <ATGENXlib.h>: include it explicitly so sketches that don't use a
// servo don't pull in the Servo library (it claims Timer1 on AVR).
//
//   #include <ATGENXlib.h>
//   #include <components/actuators/ATG_ServoMotor.h>

#include <Arduino.h>
#if defined(ESP32)
  #include <ESP32Servo.h>
#else
  #include <Servo.h>
#endif
#include "../base/ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

/**
 * Hobby servo with the same on/off vocabulary as other outputs, so rules like
 * "button toggles the door" just work:
 *
 *   ServoMotor door(9, 90, 0);   // pin, ON angle, OFF angle
 *   door.on();                   // → 90°
 *   door.write(45);              // any angle
 *   door.setLevel(knob.ratio()); // follow a potentiometer (0..1 → 0..180°)
 */
class ServoMotor : public ComponentBase {
public:
  ServoMotor(uint8_t pin, uint8_t onAngle = 90, uint8_t offAngle = 0)
  : ComponentBase(pin), _onAngle(onAngle), _offAngle(offAngle) {}

  const __FlashStringHelper* name() const override { return F("ATG_Servo"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    _servo.attach(_pin);
    write(_offAngle);
    _state = false;
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (_timerActive && elapsed(_timerStart, _timerMs)) {
      _timerActive = false;
      off();
    }
  }

  void write(int angle) {
    _angle = (uint8_t)constrain(angle, 0, 180);
    _servo.write(_angle);
  }
  uint8_t angle() const { return _angle; }

  void on()  { _state = true;  write(_onAngle); }
  void off() { _state = false; write(_offAngle); }
  void toggle() { _state ? off() : on(); }
  bool state() const { return _state; }

  /** Move to the ON angle for `ms`, then back. */
  void onFor(uint32_t ms) {
    on();
    _timerStart = nowMs();
    _timerMs = ms;
    _timerActive = true;
  }

  /** 0.0 → 0°, 1.0 → 180°. */
  void setLevel(float ratio) {
    _timerActive = false;
    write((int)(constrain(ratio, 0.0f, 1.0f) * 180.0f + 0.5f));
  }

private:
  Servo _servo;
  uint8_t _onAngle, _offAngle;
  uint8_t _angle = 0;
  bool _state = false;
  bool _timerActive = false;
  ms_t _timerStart = 0;
  uint32_t _timerMs = 0;
};

} // namespace atg
