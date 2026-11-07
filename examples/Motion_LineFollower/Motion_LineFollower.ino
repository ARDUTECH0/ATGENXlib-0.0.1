// A line follower: five infrared eyes under the front, two motors on an L298N.
// L298N: ENA → 5, IN1 → 6, IN2 → 7, IN3 → 8, IN4 → 9, ENB → 10. Eyes S1 (left) … S5 (right) → 2, 3, 4, 11, 12.
// Start slow, raise Kp until it holds the line, then Kd until the wobble stops.
#include <ATG_Motion.h>
using namespace atg;

RobotBase car(5, 6, 7, 8, 9, 10);
LineSensor eyes(2, 3, 4, 11, 12);
LineFollower follow(car, eyes);

void setup() {
  car.begin();
  eyes.begin();
}

void loop() {
  eyes.read();
  follow.update(true, 45, 0.9, 6);  // run, speed %, Kp, Kd
}
