#include "ATG_WiFi.h"

#if ATG_ENABLE_WIFI

// ── Platform WiFi headers ─────────────────────────────────────────
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #error "ATG_WiFi: unsupported platform (ESP32 or ESP8266 required)"
#endif

namespace atg {

// ─────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────
WiFiModule::WiFiModule(const char* ssid, const char* pass,
                       uint32_t reconnectMs, uint32_t connectTimeoutMs)
  : _ssid(ssid),
    _pass(pass),
    _reconnectMs(reconnectMs),
    _connectTimeoutMs(connectTimeoutMs)
{}

// ─────────────────────────────────────────────────────────────────
// setCredentials
// ─────────────────────────────────────────────────────────────────
void WiFiModule::setCredentials(const char* ssid, const char* pass) {
  _ssid = ssid;
  _pass = pass;
}

// ─────────────────────────────────────────────────────────────────
// begin
// ─────────────────────────────────────────────────────────────────
Result WiFiModule::begin(Runtime& rt) {
  (void)rt;
  _enabled = true;

  WiFi.persistent(false);        // don't write credentials to flash
  WiFi.setAutoReconnect(false);  // we handle reconnect ourselves
  WiFi.mode(WIFI_STA);

  _lastAttempt = 0;
  connect();
  return Result::Ok;
}

// ─────────────────────────────────────────────────────────────────
// connect  (non-blocking)
// ─────────────────────────────────────────────────────────────────
Result WiFiModule::connect() {
  if (!_enabled)           return Result::Error;
  if (!_ssid || !_ssid[0]) return Result::InvalidArg;

  Log::i(name(), String(F("Connecting → ")) + _ssid);

  WiFi.disconnect(false);
  WiFi.begin(_ssid, _pass);

  _connecting   = true;
  _connected    = false;
  _ip           = "";
  _attemptStart = nowMs();
  _lastAttempt  = _attemptStart;

  return Result::Ok;
}

// ─────────────────────────────────────────────────────────────────
// disconnect
// ─────────────────────────────────────────────────────────────────
void WiFiModule::disconnect() {
  WiFi.disconnect(true);
  _connecting = false;
  _connected  = false;
  _ip         = "";
}

// ─────────────────────────────────────────────────────────────────
// _publish
// ─────────────────────────────────────────────────────────────────
void WiFiModule::_publish(Runtime& rt, uint16_t type) {
  Event e{type, 0, 0};
  rt.events().publish(e);
}

// ─────────────────────────────────────────────────────────────────
// tick
// ─────────────────────────────────────────────────────────────────
void WiFiModule::tick(Runtime& rt) {
  if (!_enabled) return;

  const wl_status_t st = WiFi.status();

  // ── Just connected ────────────────────────────────────
  if (st == WL_CONNECTED) {
    if (!_connected) {
      _connected  = true;
      _connecting = false;
      _ip         = WiFi.localIP().toString();

      Log::i(name(), String(F("Connected  IP: ")) + _ip);

      _publish(rt, EVT_WIFI_CONNECTED);
      if (_onConnected) _onConnected();

      _publish(rt, EVT_WIFI_GOT_IP);
      if (_onGotIP) _onGotIP();
    }
    return;
  }

  // ── Lost connection ───────────────────────────────────
  if (_connected) {
    _connected  = false;
    _connecting = false;
    _ip         = "";

    Log::w(name(), F("Disconnected"));
    _publish(rt, EVT_WIFI_DISCONNECTED);
    if (_onDisconnected) _onDisconnected();
  }

  // ── Connect timeout ───────────────────────────────────
  if (_connecting && elapsed(_attemptStart, _connectTimeoutMs)) {
    _connecting = false;
    Log::w(name(), F("Connect timeout — will retry"));
  }

  // ── Reconnect policy ──────────────────────────────────
  if (!_connecting && elapsed(_lastAttempt, _reconnectMs)) {
    connect();
  }
}

} // namespace atg

#endif // ATG_ENABLE_WIFI