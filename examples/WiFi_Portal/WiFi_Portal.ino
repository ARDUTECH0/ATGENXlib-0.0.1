#include <ATGENXlib.h>
#include <components/connectivity/ATG_WiFiPortal.h>

using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

// هيحاول يتصل لو فيه بيانات محفوظة
WiFiPortal portal("ATGenX", "12345678");

void setup() {
  Serial.begin(115200);
  delay(300);

  rt.attachLogger(&sink, LogLevel::Info);

  portal.onConnected([]() {
    Serial.println("Device Connected to WiFi ✅");
  });

  rt.addModule(portal);
  rt.begin();
}

void loop() {
  rt.loopOnce();
}