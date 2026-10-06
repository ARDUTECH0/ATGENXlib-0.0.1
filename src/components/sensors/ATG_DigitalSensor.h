#pragma once
#include <Arduino.h>
#include "../base/ATG_DigitalInput.h"

namespace atg {

/**
 * Generic on/off sensor module: touch pad (TTP223), tilt switch, limit
 * switch, digital sound sensor, … `active()` hides the module's polarity.
 *
 *   DigitalSensor touch(4);                       // active HIGH (TTP223)
 *   DigitalSensor limit(5, true, true);           // switch to GND, internal pull-up
 *   if (limit.activated()) motor.off();
 */
class DigitalSensor : public DigitalInput {
public:
  DigitalSensor(uint8_t pin, bool activeLow = false, bool pullup = false, uint16_t debounceMs = 25)
  : DigitalInput(pin, pullup ? InputMode::Pullup : InputMode::Normal, debounceMs), _activeLow(activeLow) {}

  const __FlashStringHelper* name() const override { return F("ATG_DigitalSensor"); }

  /** True while the sensor is triggered (touched, tilted, pressed, sound detected). */
  bool active() const { return _activeLow ? isLow() : isHigh(); }
  /** True once when it becomes active. */
  bool activated() const { return edge() == (_activeLow ? Edge::Falling : Edge::Rising); }
  /** True once when it stops being active. */
  bool deactivated() const { return edge() == (_activeLow ? Edge::Rising : Edge::Falling); }

private:
  bool _activeLow;
};

} // namespace atg
