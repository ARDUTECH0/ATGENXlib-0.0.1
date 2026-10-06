// Touch pad (TTP223, active HIGH) → pin 4 toggles a relay on pin 8 on every touch.
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
DigitalSensor touch(4);
Relay1Ch relay(8);

void setup() {
  rt.addModule(touch);
  rt.addModule(relay);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  if (touch.activated()) relay.toggle();
}
