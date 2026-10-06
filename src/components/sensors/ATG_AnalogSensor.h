#pragma once
#include <Arduino.h>
#include "../base/ATG_AnalogInput.h"

namespace atg {

/**
 * Any sensor with an analog output: potentiometer, soil moisture, gas (MQ-x),
 * water level, rain. Wire the module's AO pin to an analog pin.
 *
 *   AnalogSensor knob(A0);
 *   knob.value();    // 0..1023
 *   knob.percent();  // 0..100
 *   knob.ratio();    // 0.0..1.0  (e.g. led.setLevel(knob.ratio()))
 */
class AnalogSensor : public AnalogInput {
public:
  explicit AnalogSensor(uint8_t pin, uint16_t sampleMs = 20, uint8_t smoothing = 3)
  : AnalogInput(pin, sampleMs, smoothing) {}

  const __FlashStringHelper* name() const override { return F("ATG_Analog"); }
};

/** Potentiometer — same as AnalogSensor, named for readability. */
class Potentiometer : public AnalogSensor {
public:
  explicit Potentiometer(uint8_t pin, uint16_t sampleMs = 20) : AnalogSensor(pin, sampleMs) {}
  const __FlashStringHelper* name() const override { return F("ATG_Pot"); }
};

/**
 * Light sensor (LDR module). Most LDR modules output a LOWER voltage in
 * brighter light; set brightIsHigh=true for the opposite wiring.
 *
 *   LightSensor ldr(A1);
 *   ldr.lightPercent();   // 0 = dark, 100 = bright
 *   ldr.isDark();         // lightPercent() < 30
 */
class LightSensor : public AnalogInput {
public:
  explicit LightSensor(uint8_t pin, bool brightIsHigh = false, uint16_t sampleMs = 50)
  : AnalogInput(pin, sampleMs), _brightIsHigh(brightIsHigh) {}

  const __FlashStringHelper* name() const override { return F("ATG_Light"); }

  uint8_t lightPercent() const { return _brightIsHigh ? percent() : (uint8_t)(100 - percent()); }
  float lightRatio() const { return lightPercent() / 100.0f; }
  bool isDark(uint8_t thresholdPercent = 30) const { return lightPercent() < thresholdPercent; }

private:
  bool _brightIsHigh;
};

} // namespace atg
