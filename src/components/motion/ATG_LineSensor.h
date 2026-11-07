#pragma once
#include <Arduino.h>
#include "ATG_RobotBase.h"

namespace atg {

/**
 * A row of up to five infrared eyes under a robot (TCRT5000 modules, a 5-channel line board). It reports where the line is,
 * from -100 (under the far left eye) to 100 (far right), and whether it's lost.
 *
 *   LineSensor eyes(2, 3, 4, 5, 6);   // S1 (left) … S5 (right)
 *   eyes.begin();
 *   eyes.read();                      // every pass of loop()
 *   if (!eyes.lost()) Serial.println(eyes.position());
 *
 * Fix it 5 to 10 mm above the floor and turn each eye's screw until its LED changes exactly at the line's edge.
 */
class LineSensor {
public:
  LineSensor(uint8_t s1, uint8_t s2 = ATG_NO_PIN, uint8_t s3 = ATG_NO_PIN, uint8_t s4 = ATG_NO_PIN, uint8_t s5 = ATG_NO_PIN) {
    const uint8_t all[] = {s1, s2, s3, s4, s5};
    for (uint8_t i = 0; i < 5; i++)
      if (all[i] != ATG_NO_PIN) _pins[_count++] = all[i];
    // spread evenly from the far left to the far right, whatever the number of eyes
    for (uint8_t i = 0; i < _count; i++) _weights[i] = _count == 1 ? 0 : (int)lroundf(-100.0f + 200.0f * i / (_count - 1));
  }

  void begin() {
    for (uint8_t i = 0; i < _count; i++) pinMode(_pins[i], INPUT);
  }

  /** A light line on a dark floor: the eyes see it as LOW instead of HIGH. */
  void lightLine(bool light = true) { _seen = light ? LOW : HIGH; }

  /** Reads the eyes. True while the line is under at least one of them. */
  bool read() {
    long sum = 0;
    uint8_t on = 0;
    for (uint8_t i = 0; i < _count; i++)
      if (digitalRead(_pins[i]) == _seen) {
        sum += _weights[i];
        on++;
      }
    _lost = on == 0;
    // off the line, the last place it was seen is kept: that's the way back to it
    if (on) _position = (float)sum / on;
    return !_lost;
  }

  float position() const { return _position; }
  bool lost() const { return _lost; }
  uint8_t eyes() const { return _count; }

private:
  uint8_t _pins[5] = {0};
  int _weights[5] = {0};
  uint8_t _count = 0;
  uint8_t _seen = HIGH;
  float _position = 0;
  bool _lost = false;
};

/**
 * Follows a line with PID on the eyes' position, 200 times a second. Off the line, it turns back to the side it last saw it on.
 *
 *   RobotBase car(5, 6, 7, 8, 9, 10);
 *   LineSensor eyes(2, 3, 4, 5, 6);
 *   LineFollower follow(car, eyes);
 *   // loop(): eyes.read(); follow.update(true, 45, 0.9, 6);   // run, speed %, Kp, Kd
 *
 * Start slow, raise Kp until it holds the line, then Kd until the wobble stops.
 */
class LineFollower {
public:
  LineFollower(RobotBase& base, LineSensor& eyes) : _base(base), _eyes(eyes) {}

  void update(bool run, float speed, float kp, float kd) {
    if (millis() - _t < 5) return;
    _t = millis();
    if (!run) {
      _base.stop();
      return;
    }
    if (_eyes.lost()) {
      const int spin = _lastErr < 0 ? -1 : 1;
      const int p = speed * 2.55f * 0.7f;
      _base.drive(spin * p, -spin * p);
      return;
    }
    const float err = _eyes.position();
    const float turn = kp * err + kd * (err - _lastErr);
    _lastErr = err;
    const int base = speed * 2.55f;
    _base.drive(constrain(base + (int)turn, -255, 255), constrain(base - (int)turn, -255, 255));
  }

private:
  RobotBase& _base;
  LineSensor& _eyes;
  float _lastErr = 0;
  uint32_t _t = 0;
};

} // namespace atg
