#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

FlameDigital flame(5, true); // activeLow=true في أغلب الموديولات
LED led(13);

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(flame);
  rt.addModule(led);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  if (flame.started()) {
    Serial.println("FLAME DETECTED!");
    led.on();
  }

  // إطفاء الليد لو مفيش لهب
  if (!flame.detected()) led.off();
}