// Turn the knob to dim the LED. Potentiometer middle pin → A0, LED (with resistor) → pin 9 (PWM).
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);
Potentiometer knob(A0);
LED lamp(9);

void setup() {
  Serial.begin(115200);
  rt.attachLogger(&sink);
  rt.addModule(knob);
  rt.addModule(lamp);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  lamp.setLevel(knob.ratio());
}
