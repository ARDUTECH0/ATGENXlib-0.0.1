// A stepper on an A4988 at 1/16 microstepping: two turns forward, a pause, two turns back, again and again.
// A4988: STEP → 3, DIR → 4, EN → 5. Set the motor current on the driver's screw before the first run.
#include <ATG_Motion.h>
using namespace atg;

StepperDriver motor(3, 4, 5, 2);  // STEP, DIR, EN, STEP pulse (µs)
bool forward = true;
uint32_t pausedAt = 0;

void setup() {
  motor.begin(200 * 16, 60, 120);  // steps a turn, top rpm, rpm gained each second
}

void loop() {
  // a new move starts each time `run` comes back on
  const bool run = pausedAt == 0;
  motor.update(run, forward, 100, 2);
  if (run && !motor.moving() && motor.stepsLeft() == 0 && millis() > 500) {
    pausedAt = millis();
    forward = !forward;
  }
  if (!run && millis() - pausedAt >= 1000) pausedAt = 0;
}
