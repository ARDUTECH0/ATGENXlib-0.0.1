#pragma once
#include <Arduino.h>

// FastAccelStepper drives the STEP pin from the chip's own timers (ESP32: RMT/MCPWM; AVR: Timer1 on pins 9 and 10),
// so the speed is steady and high whatever loop() is doing. It's used when the library is installed, unless the
// sketch says ATG_NO_FAST_STEPPER (the ATGenX simulation does: it watches each step on the pin). A pin it can't drive
// (an UNO's STEP not on 9 or 10) falls back to stepping from loop(), as before.
#if !defined(ATG_NO_FAST_STEPPER) && defined(__has_include)
#if __has_include(<FastAccelStepper.h>)
#include <FastAccelStepper.h>
#define ATG_FAST_STEPPER 1
#endif
#endif

namespace atg {

#ifndef ATG_NO_PIN
#define ATG_NO_PIN 255
#endif

#if ATG_FAST_STEPPER
/** One engine for every stepper in the sketch (FastAccelStepper wants a single one). */
inline FastAccelStepperEngine& atgStepperEngine() {
  static FastAccelStepperEngine engine;
  static bool ready = false;
  if (!ready) {
    engine.init();
    ready = true;
  }
  return engine;
}
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
 * With FastAccelStepper installed the pulses come from a hardware timer (usesTimer() says so).
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
#if ATG_FAST_STEPPER
    _fas = atgStepperEngine().stepperConnectToPin(_step);
    if (_fas) {
      _fas->setDirectionPin(_dir);
      if (_en != ATG_NO_PIN) {
        // EN low = held (every driver here); released between moves
        _fas->setEnablePin(_en, true);
        _fas->setAutoEnable(true);
      }
      _fas->setAcceleration((int32_t)max(1.0f, _ramp));
      return;
    }
#endif
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
#if ATG_FAST_STEPPER
    if (_fas) {
      updateTimer(run, forward, speedPercent, turns);
      return;
    }
#endif
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
  float speed() const {
#if ATG_FAST_STEPPER
    if (_fas) return fabsf(_fas->getCurrentSpeedInMilliHz() / 1000.0f);
#endif
    return _sps;
  }
  /** Steps left in a counted move. */
  long stepsLeft() const {
#if ATG_FAST_STEPPER
    if (_fas) return _counted ? labs(_fas->targetPos() - _fas->getCurrentPosition()) : 0;
#endif
    return _left;
  }
  bool moving() const {
#if ATG_FAST_STEPPER
    if (_fas) return _fas->isRunning();
#endif
    return _sps >= 1;
  }
  /** The pulses come from a hardware timer (FastAccelStepper), not from loop(). */
  bool usesTimer() const {
#if ATG_FAST_STEPPER
    return _fas != nullptr;
#else
    return false;
#endif
  }

private:
#if ATG_FAST_STEPPER
  /** The same behaviour on the timer: the library ramps, counts and stops; this tells it what's wanted when it changes. */
  void updateTimer(bool run, bool forward, float speedPercent, float turns) {
    const uint32_t now = millis();
    if (now - _tRamp < 2) return;
    _tRamp = now;
    const bool counted = turns > 0;
    const bool restart = turns != _turnsSeen || (run && !_wasOn);
    _turnsSeen = turns;
    _wasOn = run;
    const uint32_t hz = (uint32_t)max(1.0f, _top * constrain(speedPercent, 0.0f, 100.0f) / 100.0f);
    if (!run || speedPercent <= 0) {
      if (_fas->isRunning()) _fas->stopMove();
      _running = false;
      return;
    }
    // a change of direction waits for the motor to slow to a stop (it never loses its place)
    if (_running && forward != _fwd) {
      if (_fas->isRunning()) {
        _fas->stopMove();
        return;
      }
      _running = false;
    }
    if (hz != _hz) {
      _hz = hz;
      _fas->setSpeedInHz(hz);
      if (_running) _fas->applySpeedAcceleration();
    }
    if (counted) {
      _counted = true;
      if (restart) {
        _fwd = forward;
        _fas->move((forward ? 1 : -1) * lroundf(turns * _perTurn));
        _running = true;
      }
      return;
    }
    _counted = false;
    if (!_running || restart) {
      _fwd = forward;
      if (forward) _fas->runForward();
      else _fas->runBackward();
      _running = true;
    }
  }

  FastAccelStepper* _fas = nullptr;
  uint32_t _hz = 0;
  bool _running = false, _counted = false;
#endif
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
