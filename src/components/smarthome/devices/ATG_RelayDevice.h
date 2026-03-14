#pragma once
#include <Arduino.h>

#include "../ATG_DeviceRegistry.h"

namespace atg {

// Optional callback when relay state changes
using RelayStateCb = void (*)(const char* deviceId, bool isOn);

class RelayDevice : public IDevice {
public:
  // id: unique device id (e.g. "light-1")
  // pin: GPIO pin connected to relay IN
  // inverted: set true if LOW = ON (common in many relay modules)
  // bootState: initial state at startup
  RelayDevice(const char* id,
              uint8_t pin,
              bool inverted = false,
              bool bootState = false);

  // ── IDevice ─────────────────────────────────────────────
  const char* id()   const override { return _id.c_str(); }
  const char* type() const override { return "relay"; }

  void loop() override;

  // State JSON: { "isOn": true/false }
  void getStateJson(JsonObject out) override;

  // Handle commands:
  // { "cmd":"set", "value":true }
  // { "cmd":"toggle" }
  void handleCmdJson(JsonObject cmd, bool& handled) override;

  // Manifest capabilities
  void getCapsJson(JsonArray out) override;

  // ── Public Control ──────────────────────────────────────
  void set(bool on);
  void toggle();
  bool isOn() const { return _isOn; }

  void onStateChanged(RelayStateCb cb) { _onStateChanged = cb; }

private:
  String  _id;
  uint8_t _pin;
  bool    _inverted;
  bool    _isOn;

  RelayStateCb _onStateChanged = nullptr;

  void applyHardware();
};

} // namespace atg