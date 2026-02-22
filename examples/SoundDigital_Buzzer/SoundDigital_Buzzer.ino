#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

SoundDigital soundD(8, true); // activeHigh=true غالباً
ActiveBuzzer buz(11);

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(soundD);
  rt.addModule(buz);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  if (soundD.pulse()) {
    Serial.println("Sound pulse!");
    buz.beep(40, 40, 1);
  }
}