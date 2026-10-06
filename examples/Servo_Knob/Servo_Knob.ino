// The servo follows the knob (0..180°). Potentiometer → A0, servo signal → pin 9.
#include <ATGENXlib.h>
#include <components/actuators/ATG_ServoMotor.h>
using namespace atg;

Runtime rt;
Potentiometer knob(A0);
ServoMotor arm(9);

void setup() {
  rt.addModule(knob);
  rt.addModule(arm);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  arm.setLevel(knob.ratio());
}
