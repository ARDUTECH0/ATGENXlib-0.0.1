#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

LED led(13); // UNO: LED built-in on 13

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(led);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  static uint32_t last = 0;
  if (millis() - last > 500) {
    last = millis();
    led.toggle();
  }
}