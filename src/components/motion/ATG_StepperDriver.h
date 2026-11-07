#pragma once
#include <Arduino.h>

namespace atg {

#ifndef ATG_NO_PIN
#define ATG_NO_PIN 255
#endif

/**
 * A stepper motor on a STEP/DIR driver (A4988, DRV8825, TMC2208/2209, TB6600, DM542T…), without blocking loop(). It speeds
 * up and slows down at a set rate, slows to a stop before changing direction so it never loses its place, and lands a
 * counted move on its last step.
 *
 *   StepperDriver motor(3, 4, 5);       // STEP, DIR, EN (EN is optional)
 *   motor.begin(3200, 60, 120);         // steps a turn (200 × 1/16), top rpm, rpm gained each second
 *   motor.update(true, true, 100, 2);   // every pass of loop(): run, forward, speed %, turns (0 = keep turning)
 *
 * EN HIGH leaves the motor free on every driver here; it's held only while it moves.
 */
class StepperDriver {
public:
  /** `pulseUs`: the shortest STEP pulse the driver reliably sees (2 on an A4988, 5 on the boxed drivers). */
  StepperDriver(uint8_t stepPin, uint8_t dirPin, uint8_t enPin = ATG_NO_PIN, uint8_t pulseUs = 5)
  : _step(stepPin), _dir(dirPin), _en(enPin), _pulseUs(pulseUs) {}

  void begin(float stepsPerTurn, float topRpm, float accelRpm) {
    _perTurn = max(1.0f, stepsPerTurn);
    _top = max(1.0f, topRpm) * _perTurn / 60.0f;
    _ramp = max(1.0f, accelRpm) * _perTurn / 60.0f;
    _creep = max(20.0f, _ramp * 0.02f);
    pinMode(_step, OUTPUT);
    pinMode(_dir, OUTPUT);
    digitalWrite(_dir, HIGH);
    _fwd = true;
    if (_en != ATG_NO_PIN) {
      pinMode(_en, OUTPUT);
      digitalWrite(_en, HIGH);
    }
  }

  /**
   * Call on every pass of loop(). `turns` > 0 is a move of that many turns: a new number starts one, and so does switching
   * `run` back on. 0 keeps it turning.
   */
  void update(bool run, bool forward, float speedPercent, float turns) {
    const uint32_t now = micros();
    // the sums run 500 times a second, so the loop itself stays quick enough to pulse on time
    if (now - _tRamp >= 2000) {
      const float gain = _ramp * (now - _tRamp) / 1000000.0f;
      _tRamp = now;
      if (turns != _turnsSeen || (run && !_wasOn)) {
        _turnsSeen = turns;
        _left = lroundf(turns * _perTurn);
      }
      _wasOn = run;
      const bool counted = turns > 0;
      const bool go = run && (!counted || _left > 0);
      float target = (go && forward == _fwd) ? _top * constrain(speedPercent, 0.0f, 100.0f) / 100.0f : 0;
      // a counted move brakes in time to land on its last step, not past it
      if (counted && target > _creep && _sps * _sps >= 2.0f * _ramp * _left) target = _creep;
      _sps += constrain(target - _sps, -gain, gain);
      if (counted && _left == 0) _sps = 0;
      if (_sps < 1 && forward != _fwd) {
        _fwd = forward;
        digitalWrite(_dir, forward ? HIGH : LOW);
        delayMicroseconds(10);
      }
      _period = _sps >= 1 ? (uint32_t)(1000000.0f / _sps) : 0;
      if (_en != ATG_NO_PIN) digitalWrite(_en, (go || _sps >= 1) ? LOW : HIGH);
    }
    if (_period && now - _tStep >= _period) {
      // each step is timed from the last one's due time, so a slow pass of the loop doesn't lower the speed
      _tStep = now - _tStep >= 2 * _period ? now : _tStep + _period;
      digitalWrite(_step, HIGH);
      delayMicroseconds(_pulseUs);
      digitalWrite(_step, LOW);
      if (_left > 0 && --_left == 0 && turns > 0) {
        _period = 0;
        _sps = 0;
      }
    }
  }

  /** Steps a second right now. */
  float speed() const { return _sps; }
  /** Steps left in a counted move. */
  long stepsLeft() const { return _left; }
  bool moving() const { return _sps >= 1; }

private:
  uint8_t _step, _dir, _en, _pulseUs;
  float _perTurn = 200, _top = 200, _ramp = 400, _creep = 20;
  float _sps = 0;
  bool _fwd = true;
  uint32_t _tStep = 0, _tRamp = 0, _period = 0;
  long _left = 0;
  float _turnsSeen = 0;
  bool _wasOn = false;
};

} // namespace atg
