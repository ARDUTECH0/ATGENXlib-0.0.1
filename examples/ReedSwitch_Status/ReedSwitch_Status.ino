#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

ReedSwitch reed(6, true); // pullup=true
LED led(13);

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(reed);
  rt.addModule(led);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  if (reed.closed()) {
    led.on();
    Serial.println("Reed CLOSED (magnet near)");
  } else {
    led.off();
  }

  delay(50); // فقط لتقليل السيريال spam
}