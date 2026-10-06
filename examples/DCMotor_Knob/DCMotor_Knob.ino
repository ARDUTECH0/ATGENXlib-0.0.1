// The knob sets the motor speed. L298N: ENA → pin 9 (PWM), IN1 → pin 8 (IN2 to GND).
// Potentiometer → A0.
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
Potentiometer knob(A0);
DCMotor fan(9, 8);

void setup() {
  rt.addModule(knob);
  rt.addModule(fan);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  fan.setLevel(knob.ratio());
}
