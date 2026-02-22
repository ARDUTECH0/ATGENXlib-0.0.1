#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

ActiveBuzzer buz(11);

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(buz);
  rt.begin();

  // أول ما يشتغل: 3 beeps
  buz.beep(120, 120, 3);
}

void loop() {
  rt.loopOnce();

  // كل 5 ثواني: beep واحد
  static uint32_t last = 0;
  if (millis() - last > 5000) {
    last = millis();
    buz.beep(80, 80, 1);
    Serial.println("Beep!");
  }
}