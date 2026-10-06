// Buzzer sounds when the board is tilted more than 30 degrees. MPU-6050 on I2C (0x68), buzzer → pin 8.
#include <ATGENXlib.h>
#include <components/sensors/ATG_MPU6050.h>
using namespace atg;

Runtime rt;
MPU6050Sensor imu;
ActiveBuzzer buzzer(8);

void setup() {
  Serial.begin(115200);
  rt.addModule(imu);
  rt.addModule(buzzer);
  rt.begin();
}

void loop() {
  rt.loopOnce();
  if (fabs(imu.tiltX()) > 30 || fabs(imu.tiltY()) > 30) buzzer.on();
  else buzzer.off();
}
