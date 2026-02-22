#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

DHTSensor dht(4, DhtType::DHT11, 2000); // pin=4, type=DHT11, sample every 2s

void setup() {
  Serial.begin(115200);
  delay(300);

  rt.attachLogger(&sink);
  rt.addModule(dht);
  rt.begin();
}

void loop() {
  rt.loopOnce();

  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial.print("TempC: "); Serial.print(dht.temperatureC());
    Serial.print(" | Hum: "); Serial.println(dht.humidity());
  }
}