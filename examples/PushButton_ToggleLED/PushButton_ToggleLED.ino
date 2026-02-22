#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

LED led(13);
PushButton btn(2); // pullup افتراضي

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(led);
  rt.addModule(btn);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  if (btn.pressed()) {
    led.toggle();
    Serial.println("Pressed -> LED toggled");
  }
}