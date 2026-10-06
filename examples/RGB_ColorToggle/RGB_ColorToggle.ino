// Each button press toggles an orange light. RGB LED (common cathode): R → 9, G → 10, B → 11.
// Button between pin 2 and GND.
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
RGBLed mood(9, 10, 11, false, 255, 80, 0);
PushButton btn(2);

void setup() {
  rt.addModule(mood);
  rt.addModule(btn);
  btn.bind(1, [] { mood.toggle(); });
  rt.begin();
}

void loop() {
  rt.loopOnce();
}
