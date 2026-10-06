// The stepper follows the knob over one revolution. 28BYJ-48 + ULN2003: IN1..IN4 → pins 8..11.
// Potentiometer → A0.
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
Potentiometer knob(A0);
StepperMotor arm(8, 9, 10, 11);

void setup() {
  Serial.begin(115200);
  rt.addModule(knob);
  rt.addModule(arm);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  arm.setLevel(knob.ratio());
}
