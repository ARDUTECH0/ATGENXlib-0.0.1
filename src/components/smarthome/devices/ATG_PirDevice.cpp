#if ATG_SMARTHOME_ONLY

#include "ATG_PirDevice.h"
 
namespace atg {

PirDevice::PirDevice(const char* id, uint8_t pin,
                     bool activeHigh,
                     uint16_t debounceMs,
                     uint32_t cooldownMs)
  : _id(id),
    _pin(pin),
    _activeHigh(activeHigh),
    _debounceMs(debounceMs),
    _cooldownMs(cooldownMs) {

  pinMode(_pin, INPUT);
  _lastRaw = readRaw();
  _lastRawChangeMs = millis();
}

bool PirDevice::readRaw() const {
  bool v = (digitalRead(_pin) == HIGH);
  return _activeHigh ? v : !v;
}

void PirDevice::setMotion(bool next) {
  if (_motion == next) return;

  _motion = next;

  Serial.printf("[PirDevice] %s motion=%d\n", _id.c_str(), _motion); // debug
  notifyStateChanged();   // مهم جدًا

  if (_onMotionChanged) _onMotionChanged(_id.c_str(), _motion);
}
void PirDevice::loop() {
  uint32_t now = millis();
  bool raw = readRaw();

  // Track raw changes
  if (raw != _lastRaw) {
    _lastRaw = raw;
    _lastRawChangeMs = now;
  }

  // Debounce: accept raw value only if stable for debounceMs
  if (_debounceMs > 0) {
    if ((uint32_t)(now - _lastRawChangeMs) >= _debounceMs) {
      setMotion(_lastRaw);
    }
  } else {
    setMotion(_lastRaw);
  }
}

void PirDevice::getStateJson(JsonObject out) {
  out["motion"] = _motion;
}

void PirDevice::getCapsJson(JsonArray out) {
  out.add("read");
  out.add("event"); // indicates it can emit motion events
}

void PirDevice::handleCmdJson(JsonObject, bool& handled) {
  handled = false; // read-only device
}

} // namespace atg
#endif