#include "ATG_WiFi.h"

#if ATG_ENABLE_WIFI

  #if defined(ESP32)
    #include <WiFi.h>
  #else
    // لو حد فعل WiFi على بورد غير ESP32
  #endif

namespace atg {

WiFiModule::WiFiModule(const char* ssid, const char* pass, uint32_t reconnectMs, uint32_t connectTimeoutMs)
: _ssid(ssid), _pass(pass), _reconnectMs(reconnectMs), _connectTimeoutMs(connectTimeoutMs) {}

void WiFiModule::setCredentials(const char* ssid, const char* pass) {
  _ssid = ssid;
  _pass = pass;
}

Result WiFiModule::begin(Runtime& rt) {
#if !defined(ESP32)
  (void)rt;
  Log::e(F("ATG_WiFi"), "WiFi enabled but board is not ESP32");
  _enabled = false;
  return Result::Error;
#else
  (void)rt;
  _enabled = true;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); // هنمشي reconnect logic بتاعتنا

  // أول محاولة اتصال مباشرة
  _lastAttempt = 0;
  connect();
  return Result::Ok;
#endif
}

Result WiFiModule::connect() {
#if !defined(ESP32)
  return Result::Error;
#else
  if (!_enabled) return Result::Error;
  if (!_ssid || !_ssid[0]) return Result::InvalidArg;

  Log::i(F("ATG_WiFi"), String("Connecting to: ") + _ssid);
  _connecting = true;
  _connected = false;
  _ip = "";

  _attemptStart = nowMs();
  _lastAttempt = _attemptStart;

  WiFi.begin(_ssid, _pass);
  return Result::Ok;
#endif
}

void WiFiModule::disconnect() {
#if defined(ESP32)
  WiFi.disconnect(true);
#endif
  _connecting = false;
  _connected = false;
  _ip = "";
}

void WiFiModule::publish(Runtime& rt, uint16_t type) {
  Event e{type, 0, 0};
  rt.events().publish(e);
}

void WiFiModule::tick(Runtime& rt) {
#if !defined(ESP32)
  (void)rt;
  return;
#else
  if (!_enabled) return;

  const wl_status_t st = WiFi.status();

  // Connected
  if (st == WL_CONNECTED) {
    if (!_connected) {
      _connected = true;
      _connecting = false;

      _ip = WiFi.localIP().toString();
      Log::i(F("ATG_WiFi"), String("Connected, IP: ") + _ip);

      publish(rt, EVT_WIFI_CONNECTED);
      if (_onConnected) _onConnected();

      publish(rt, EVT_WIFI_GOT_IP);
      if (_onGotIP) _onGotIP();
    }
    return;
  }

  // Not connected
  if (_connected) {
    _connected = false;
    _connecting = false;
    _ip = "";
    Log::e(F("ATG_WiFi"), "Disconnected");

    publish(rt, EVT_WIFI_DISCONNECTED);
    if (_onDisconnected) _onDisconnected();
  }

  // Timeout while connecting
  if (_connecting && elapsed(_attemptStart, _connectTimeoutMs)) {
    _connecting = false;
    Log::e(F("ATG_WiFi"), "Connect timeout");
    // هنعيد المحاولة بعد reconnectMs
  }

  // Reconnect policy
  if (!_connecting && elapsed(_lastAttempt, _reconnectMs)) {
    connect();
  }
#endif
}

} // namespace atg

#else 
#endif