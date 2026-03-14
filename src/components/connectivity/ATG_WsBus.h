#pragma once

// arduinoWebSockets (Links2004) works on ESP32 and ESP8266
#if defined(ESP32) || defined(ESP8266)

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

#include "../smarthome/ATG_DeviceRegistry.h"

namespace atg {

class WsBus {
public:
  WsBus() = default;

  // ── Must call before begin() ───────────────────────────
  void setRegistry         (DeviceRegistry* r)        { _reg      = r;  }
  void setAuthTokenProvider(String (*fn)())            { _getToken = fn; }

  // ── Lifecycle ─────────────────────────────────────────
  bool begin(uint16_t port = 8080);
  void loop();

  // ── Broadcast state to all connected clients ──────────
  void broadcastState(const char* deviceId, JsonObject state);

  // ── Destructor — free WebSocketsServer ────────────────
  ~WsBus() {
    if (_ws) {
      _ws->close();
      delete _ws;
      _ws = nullptr;
    }
  }

  // non-copyable
  WsBus(const WsBus&)            = delete;
  WsBus& operator=(const WsBus&) = delete;

private:
  WebSocketsServer* _ws      = nullptr;
  uint16_t          _port    = 0;

  DeviceRegistry*   _reg      = nullptr;
  String          (*_getToken)() = nullptr;

  void onEvent   (uint8_t num, WStype_t type, uint8_t* payload, size_t len);
  bool checkToken(JsonObject root);
};

} // namespace atg

#endif // ESP32 || ESP8266