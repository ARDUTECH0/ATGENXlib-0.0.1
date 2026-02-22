#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

// ===== Sensors =====
DHTSensor dht(4, DhtType::DHT11);         // DHT pin 4
PushButton btn(2, true);                  // Button pin 2 pullup
PIRMotion pir(3);                         // PIR pin 3
FlameDigital flame(5, true);              // Flame pin 5 active LOW
ReedSwitch reed(6, true);                 // Reed pin 6 pullup
IrObstacle irObs(7, true);                // IR obstacle pin 7 active LOW
SoundDigital soundD(8, true);             // Sound digital pin 8 active HIGH

// ===== Actuators =====
LED led(9);                               // LED pin 9 (PWM)
Relay1Ch relay(10, true);                 // Relay pin 10 active LOW
ActiveBuzzer buz(11);                     // Buzzer pin 11

void setup() {
  Serial.begin(115200);
  delay(300);

  rt.attachLogger(&sink, LogLevel::Info);
  Log::i(F("ATG"), String("Board: ") + (const __FlashStringHelper*)boardName());

  // add modules
  rt.addModule(dht);
  rt.addModule(btn);
  rt.addModule(pir);
  rt.addModule(flame);
  rt.addModule(reed);
  rt.addModule(irObs);
  rt.addModule(soundD);

  rt.addModule(led);
  rt.addModule(relay);
  rt.addModule(buz);

  rt.begin();
}

void loop() {
  rt.loopOnce();

  // Button -> Toggle LED
  if (btn.pressed()) {
    led.toggle();
    buz.beep(60, 60, 1);
  }

  // PIR -> Relay ON while motion
  if (pir.motionStarted()) relay.on();
  if (pir.motionEnded())   relay.off();

  // Flame detected -> beep fast
  if (flame.started()) buz.beep(80, 80, 3);

  // IR obstacle -> LED brightness
  if (irObs.detected()) led.setBrightness(255);
  else led.setBrightness(40);

  // Sound pulse -> short beep
  if (soundD.pulse()) buz.beep(40, 40, 1);

  // Print DHT every ~2s (it updates internally)
  static ms_t lastPrint = 0;
  if (elapsed(lastPrint, 2000)) {
    lastPrint = nowMs();
    Serial.print("T="); Serial.print(dht.temperatureC());
    Serial.print(" H="); Serial.println(dht.humidity());
  }
}