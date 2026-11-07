#pragma once
#include <Arduino.h>

namespace atg {

#ifndef ATG_NO_PIN
#define ATG_NO_PIN 255
#endif

/**
 * Two DC motors on one H-bridge module (L298N, L293D, TB6612FNG): a robot's left and right wheels.
 * Each side takes a power from -255 (full backward) to 255 (full forward).
 *
 *   RobotBase car(5, 6, 7, 8, 9, 10);   // ENA, IN1, IN2, IN3, IN4, ENB
 *   car.begin();
 *   car.drive(200, 200);                // straight ahead
 *   car.update(true, 60, -20);          // from an app: run, throttle and steer, -100 … 100
 *
 * If one motor turns the wrong way, swap its two wires on the driver.
 */
class RobotBase {
public:
  RobotBase(uint8_t ena, uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4, uint8_t enb)
  : _ena(ena), _in1(in1), _in2(in2), _in3(in3), _in4(in4), _enb(enb) {}

  void begin() {
    const uint8_t pins[] = {_ena, _in1, _in2, _in3, _in4, _enb};
    for (uint8_t i = 0; i < 6; i++) pinMode(pins[i], OUTPUT);
    stop();
  }

  /** -255 … 255 for each side. */
  void drive(int left, int right) {
    _left = constrain(left, -255, 255);
    _right = constrain(right, -255, 255);
    motor(_ena, _in1, _in2, _left);
    motor(_enb, _in3, _in4, _right);
  }

  void stop() { drive(0, 0); }

  /**
   * Driven by hand, 50 times a second: throttle and steer from -100 to 100, as an app's sliders send them. The steering is
   * added to one side and taken from the other. `run` false stops the wheels.
   */
  void update(bool run, float throttle, float steer) {
    if (millis() - _t < 20) return;
    _t = millis();
    if (!run) {
      stop();
      return;
    }
    const int power = throttle * 2.55f;
    const int turn = steer * 1.5f;
    drive(constrain(power + turn, -255, 255), constrain(power - turn, -255, 255));
  }

  int left() const { return _left; }
  int right() const { return _right; }

private:
  static void motor(uint8_t en, uint8_t a, uint8_t b, int power) {
    digitalWrite(a, power > 0 ? HIGH : LOW);
    digitalWrite(b, power < 0 ? HIGH : LOW);
    analogWrite(en, abs(power));
  }

  uint8_t _ena, _in1, _in2, _in3, _in4, _enb;
  int _left = 0, _right = 0;
  uint32_t _t = 0;
};

} // namespace atg
