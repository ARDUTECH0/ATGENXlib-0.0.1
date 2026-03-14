#if ATG_SMARTHOME_ONLY

#include "ATG_RelayDevice.h"

namespace atg {

RelayDevice::RelayDevice(const char* id, uint8_t pin, bool inverted, bool bootState)
  : _id(id), _pin(pin), _inverted(inverted), _isOn(bootState) {
  pinMode(_pin, OUTPUT);
  applyHardware();
}

void RelayDevice::loop() {
  // no periodic work
}

void RelayDevice::applyHardware() {
  bool levelOn = _inverted ? LOW : HIGH;
  bool levelOff = _inverted ? HIGH : LOW;
  digitalWrite(_pin, _isOn ? levelOn : levelOff);
}

void RelayDevice::set(bool on) {
  if (_isOn == on) return;
  _isOn = on;
  applyHardware();
  if (_onStateChanged) _onStateChanged(_id.c_str(), _isOn);
}

void RelayDevice::toggle() { set(!_isOn); }

void RelayDevice::getStateJson(JsonObject out) {
  out["isOn"] = _isOn;
}

void RelayDevice::getCapsJson(JsonArray out) {
  out.add("set");
  out.add("toggle");
}

void RelayDevice::handleCmdJson(JsonObject cmd, bool& handled) {
  handled = false;
  const char* c = cmd["cmd"] | "";

  if (!strcmp(c, "set")) {
    bool v = cmd["value"] | false;
    set(v);
    handled = true;
    return;
  }

  if (!strcmp(c, "toggle")) {
    toggle();
    handled = true;
    return;
  }
}

} // namespace atg
#endif