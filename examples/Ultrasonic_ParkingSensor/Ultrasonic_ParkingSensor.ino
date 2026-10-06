// Parking sensor: the buzzer sounds when something is closer than 20 cm.
// HC-SR04: TRIG → pin 7, ECHO → pin 6. Active buzzer → pin 8.
#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);
Ultrasonic eye(7, 6);
ActiveBuzzer buzzer(8);

void setup() {
  Serial.begin(115200);
  rt.attachLogger(&sink);
  rt.addModule(eye);
  rt.addModule(buzzer);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  if (eye.inRange() && eye.distanceCm() < 20) buzzer.on();
  else buzzer.off();
}
