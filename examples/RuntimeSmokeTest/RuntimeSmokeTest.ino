#include <ATGENXlib.h>
using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

void setup() {
  Serial.begin(115200);
  delay(300);

  rt.attachLogger(&sink, LogLevel::Info);
  Log::i(F("ATG"), String("Board: ") + (const __FlashStringHelper*)boardName());

  rt.begin();
}

void loop() {
  rt.loopOnce();
}