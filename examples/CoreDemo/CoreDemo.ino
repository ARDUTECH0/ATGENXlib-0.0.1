#include <ATGENXlib.h>

using namespace atg;

Runtime rt;
SerialLogSink sink(Serial);

static void onPing(const Event& e) {
  Log::i(F("APP"), String("PING received a=") + e.a + " b=" + e.b);
}

enum : uint16_t {
  EVT_PING = 1,
};

static void taskPing() {
  Event e{EVT_PING, nowMs(), 123};
  rt.events().publish(e);
}

StateMachine sm;

static void onIdleEnter() { Log::i(F("SM"), "Enter IDLE"); }
static void onIdleTick()  { /* nothing */ }
static void onRunEnter()  { Log::i(F("SM"), "Enter RUN");  }

void setup() {
  Serial.begin(115200);
  delay(300);

  rt.attachLogger(&sink, LogLevel::Debug);

  Log::i(F("ATG"), String("Board: ") + (const __FlashStringHelper*)boardName());

  rt.events().subscribe(EVT_PING, onPing);
  rt.scheduler().every(1000, taskPing, true);

  sm.add(1, F("IDLE"), onIdleEnter, onIdleTick, nullptr);
  sm.add(2, F("RUN"),  onRunEnter,  nullptr,    nullptr);
  sm.set(1);

  rt.begin();
}

void loop() {
  rt.loopOnce();
  sm.tick();

  // Example transition
  if (nowMs() > 5000 && sm.current() != 2) sm.set(2);
}