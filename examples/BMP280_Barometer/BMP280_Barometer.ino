// Prints temperature, pressure and altitude from a BMP280 (I2C, address 0x76).
#include <ATGENXlib.h>
#include <components/sensors/ATG_BMP280.h>
using namespace atg;

Runtime rt;
BMP280Sensor air;

void setup() {
  Serial.begin(115200);
  rt.addModule(air);
  rt.begin();
  if (!air.ok()) Serial.println("BMP280 not found - check wiring/address");
}

void loop() {
  rt.loopOnce();
  static unsigned long last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.print("T=");
    Serial.print(air.temperatureC(), 2);
    Serial.print(" P=");
    Serial.print(air.pressureHpa(), 2);
    Serial.print(" alt=");
    Serial.println(air.altitudeM(), 1);
  }
}
