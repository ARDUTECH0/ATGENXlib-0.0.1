// Night light: the LED turns on when it gets dark. LDR module AO → A0, LED → pin 13.
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);
LightSensor ldr(A0);
LED lamp(13);

void setup() {
  Serial.begin(115200);
  rt.attachLogger(&sink);
  rt.addModule(ldr);
  rt.addModule(lamp);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  if (ldr.isDark()) lamp.on(); else lamp.off();
}
