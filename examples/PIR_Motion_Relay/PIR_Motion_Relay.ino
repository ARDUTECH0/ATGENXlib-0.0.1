#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

PIRMotion pir(3);
Relay1Ch relay(10, true); // activeLow غالباً true

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(pir);
  rt.addModule(relay);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  if (pir.motionStarted()) {
    relay.on();
    Serial.println("Motion -> Relay ON");
  }

  if (pir.motionEnded()) {
    relay.off();
    Serial.println("No motion -> Relay OFF");
  }
}