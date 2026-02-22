#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

IrObstacle irObs(7, true); // activeLow غالباً true
LED led(9);                // PWM pin

void setup() {
  Serial.begin(115200);
  delay(200);

  rt.attachLogger(&sink);
  rt.addModule(irObs);
  rt.addModule(led);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  if (irObs.detected()) {
    led.setBrightness(255);
    Serial.println("Obstacle detected");
  } else {
    led.setBrightness(30);
  }

  delay(30);
}