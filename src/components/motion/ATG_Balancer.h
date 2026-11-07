#pragma once
#include <Arduino.h>
#include "ATG_RobotBase.h"
#include "ATG_TiltSensor.h"

namespace atg {

/**
 * Keeps a two-wheeled robot standing: PID on the tilt, 200 times a second. Leaning a little forward is how it drives
 * forward, so the app's throttle moves the angle it holds.
 *
 *   RobotBase car(9, 8, 7, 10, 11, 12);
 *   TiltSensor tilt;
 *   Balancer balance(car, tilt);
 *   // setup(): car.begin(); tilt.begin(); balance.begin(0, 30, 15);
 *   // loop():  tilt.update(); balance.update(true, 0, 0, 25, 0.4, 0.8);   // run, throttle, steer, Kp, Ki, Kd
 *
 * It lets go of the wheels past 35 degrees. Hold it nearly upright at power-on and let go after a second.
 */
class Balancer {
public:
  Balancer(RobotBase& base, TiltSensor& tilt) : _base(base), _tilt(tilt) {}

  /**
   * `level`: the angle it stands upright at. `minPower`: the power below which the wheels don't move at all (0–120).
   * `hold`: how hard it leans against its own creep (0–40): with no wheel encoders, the power it keeps asking for shows how
   * fast it's creeping, and leaning against that stops a slow drift growing until the motors run out of speed.
   */
  void begin(float level = 0, int minPower = 30, float hold = 15) {
    _level = level;
    _minPower = constrain(minPower, 0, 120);
    _hold = constrain(hold, 0.0f, 40.0f);
  }

  void update(bool run, float throttle, float steer, float kp, float ki, float kd) {
    if (micros() - _t < 5000) return;
    _t = micros();
    if (!run || fabs(_tilt.angle()) > 35) {
      _sum = 0;
      _drift = 0;
      _power = 0;
      _base.stop();
      return;
    }
    _drift = _drift * 0.995f + (_power / 255.0f) * 0.005f;
    const float err = _tilt.angle() - _level - throttle * 0.03f + _hold * _drift;
    _sum = constrain(_sum + err * 0.005f, -60.0f, 60.0f);
    const float out = kp * err + ki * _sum + kd * _tilt.rate();
    _power = constrain(out > 0 ? out + _minPower : out < 0 ? out - _minPower : 0, -255.0f, 255.0f);
    const int turn = steer * 0.6f;
    _base.drive(constrain(_power + turn, -255, 255), constrain(_power - turn, -255, 255));
  }

  /** The power the wheels are getting, -255 … 255. */
  int power() const { return _power; }

private:
  RobotBase& _base;
  TiltSensor& _tilt;
  float _level = 0;
  int _minPower = 30;
  float _hold = 15;
  float _sum = 0;
  float _drift = 0;
  int _power = 0;
  uint32_t _t = 0;
};

} // namespace atg
