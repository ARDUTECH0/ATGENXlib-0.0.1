#if ATG_SMARTHOME_ONLY

#pragma once
#include <Arduino.h>
#include "../ATG_DeviceRegistry.h"

namespace atg {

using MotionCb = void (*)(const char* deviceId, bool motion);

class PirDevice : public IDevice {
public:
  PirDevice(const char* id, uint8_t pin,
            bool activeHigh = true,
            uint16_t debounceMs = 60,
            uint32_t cooldownMs = 800);

  const char* id()   const override { return _id.c_str(); }
  const char* type() const override { return "pir"; }

  void loop() override;

  void getStateJson(JsonObject out) override;
  void handleCmdJson(JsonObject cmd, bool& handled) override; // read-only
  void getCapsJson(JsonArray out) override;

  // Optional hook so Hub can broadcast events/states
  void onMotionChanged(MotionCb cb) { _onMotionChanged = cb; }

  bool motion() const { return _motion; }

private:
  String _id;
  uint8_t _pin;
  bool _activeHigh;

  uint16_t _debounceMs;
  uint32_t _cooldownMs;

  bool _motion = false;          // stable state
  bool _lastRaw = false;         // raw read
  uint32_t _lastRawChangeMs = 0; // for debounce
  uint32_t _lastEventMs = 0;     // cooldown

  MotionCb _onMotionChanged = nullptr;

  bool readRaw() const;
  void setMotion(bool next);
};

} // namespace atg
#endif