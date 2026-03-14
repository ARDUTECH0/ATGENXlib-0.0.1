#pragma once
#include <Arduino.h>
#include "../ATG_DeviceRegistry.h"

namespace atg {

class RelayDevice : public IDevice {
public:
  RelayDevice(const char* id, uint8_t pin, bool inverted = false);

  const char* id() const override { return _id.c_str(); }
  const char* type() const override { return "relay"; }

  void loop() override;
  void getStateJson(JsonObject out) override;
  void handleCmdJson(JsonObject cmd, bool& handled) override;
  void getCapsJson(JsonArray out) override;

  void set(bool on);

private:
  String _id;
  uint8_t _pin;
  bool _inv;
  bool _isOn = false;
};

} // namespace atg