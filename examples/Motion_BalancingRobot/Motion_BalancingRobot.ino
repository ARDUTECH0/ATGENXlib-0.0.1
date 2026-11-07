// A robot that stands on two wheels: an MPU6050 on SDA/SCL, two gearmotors on an L298N.
// L298N: ENA → 9, IN1 → 8, IN2 → 7, IN3 → 4, IN4 → 2, ENB → 10.
// Hold it nearly upright at power-on and let go after a second: it learns the gyro's zero first.
// The PID values are a start: raise Kp until it fights the fall, then Kd until the shaking calms, then a little Ki.
#include <ATG_Motion.h>
using namespace atg;

RobotBase car(9, 8, 7, 4, 2, 10);
TiltSensor tilt;
Balancer balance(car, tilt);

void setup() {
  car.begin();
  tilt.begin();
  balance.begin(0, 30, 15);  // upright angle, power that starts the wheels, staying in place
}

void loop() {
  tilt.update();
  balance.update(true, 0, 0, 25, 0.4, 0.8);  // run, throttle, steer, Kp, Ki, Kd
}
