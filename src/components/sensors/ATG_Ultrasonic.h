#pragma once
#include <Arduino.h>
#include "../base/ATG_ComponentBase.h"
#include "../../core/ATG_Time.h"

namespace atg {

/**
 * HC-SR04 ultrasonic distance sensor.
 *
 *   Ultrasonic eye(7, 6);          // TRIG pin, ECHO pin
 *   eye.distanceCm();              // last reading, 0 when out of range
 *   if (eye.inRange() && eye.distanceCm() < 20) buzzer.on();
 *
 * Each measurement waits at most for the echo of `maxCm` (≈23 ms at 400 cm),
 * once every `sampleMs`.
 */
class Ultrasonic : public ComponentBase {
public:
  Ultrasonic(uint8_t trigPin, uint8_t echoPin, uint16_t sampleMs = 100, uint16_t maxCm = 400)
  : ComponentBase(trigPin), _echo(echoPin), _sampleMs(sampleMs), _maxCm(maxCm) {}

  const __FlashStringHelper* name() const override { return F("ATG_Ultrasonic"); }

  Result begin(Runtime& rt) override {
    (void)rt;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    pinMode(_echo, INPUT);
    _last = nowMs() - _sampleMs;
    return Result::Ok;
  }

  void tick(Runtime& rt) override {
    (void)rt;
    if (!elapsed(_last, _sampleMs)) return;
    _last = nowMs();

    digitalWrite(_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_pin, LOW);

    const unsigned long timeoutUs = (unsigned long)_maxCm * 58UL + 1000UL;
    const unsigned long us = pulseIn(_echo, HIGH, timeoutUs);
    if (us == 0) {
      _valid = false;
      _cm = 0;
    } else {
      _valid = true;
      _cm = us / 58.0f;
    }
  }

  /** Distance in cm of the last valid echo; 0 if nothing was in range. */
  float distanceCm() const { return _cm; }
  bool inRange() const { return _valid; }
  /** 0.0 (touching) .. 1.0 (maxCm or nothing in range). */
  float ratio() const { return _valid ? min(_cm / (float)_maxCm, 1.0f) : 1.0f; }

private:
  uint8_t _echo;
  uint16_t _sampleMs;
  uint16_t _maxCm;
  float _cm = 0;
  bool _valid = false;
  ms_t _last = 0;
};

} // namespace atg
