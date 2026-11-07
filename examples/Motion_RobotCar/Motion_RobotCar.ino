// A car you drive from the Serial Monitor: w forward, s back, a left, d right, any other key stops it.
// L298N: ENA → 5, IN1 → 6, IN2 → 7, IN3 → 8, IN4 → 9, ENB → 10. Power the motors from a battery, never the board's 5 V.
#include <ATG_Motion.h>
using namespace atg;

RobotBase car(5, 6, 7, 8, 9, 10);
float throttle = 0, steer = 0;

void setup() {
  Serial.begin(115200);
  car.begin();
}

void loop() {
  if (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n' || c == '\r') return;
    throttle = c == 'w' ? 70 : c == 's' ? -70 : 0;
    steer = c == 'd' ? 50 : c == 'a' ? -50 : 0;
  }
  car.update(true, throttle, steer);  // run, throttle and steer from -100 to 100
}
