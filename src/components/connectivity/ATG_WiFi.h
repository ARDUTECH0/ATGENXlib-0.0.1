#if defined(ESP32) || defined(ESP8266)

#pragma once
#include <Arduino.h>

#include "../../core/ATG_Config.h"
#include "../../core/ATG_Types.h"
#include "../../core/ATG_Time.h"
#include "../../core/ATG_Log.h"
#include "../../core/ATG_Module.h"
#include "../../core/ATG_Runtime.h"
#include "../../core/ATG_EventBus.h"

#if ATG_ENABLE_WIFI

namespace atg {

// ── Events ────────────────────────────────────────────────────────
enum : uint16_t {
  EVT_WIFI_CONNECTED    = 2001,
  EVT_WIFI_DISCONNECTED = 2002,
  EVT_WIFI_GOT_IP       = 2003,
};

using WiFiCb = void (*)();

// ── WiFiModule ────────────────────────────────────────────────────
class WiFiModule : public Module {
public:
  WiFiModule(const char* ssid,
             const char* pass,
             uint32_t reconnectMs      = 5000,
             uint32_t connectTimeoutMs = 15000);

  const __FlashStringHelper* name() const override { return F("ATG_WiFi"); }

  Result begin(Runtime& rt) override;
  void   tick (Runtime& rt) override;

  // ── Actions ───────────────────────────────────────────
  Result connect();
  void   disconnect();

  // ── Status ────────────────────────────────────────────
  bool   isEnabled()   const { return _enabled;   }
  bool   isConnected() const { return _connected; }
  String ip()          const { return _ip;        }

  // ── Callbacks ─────────────────────────────────────────
  void onConnected   (WiFiCb cb) { _onConnected    = cb; }
  void onDisconnected(WiFiCb cb) { _onDisconnected = cb; }
  void onGotIP       (WiFiCb cb) { _onGotIP        = cb; }

  // ── Credentials ───────────────────────────────────────
  void setCredentials(const char* ssid, const char* pass);

private:
  const char* _ssid;
  const char* _pass;

  uint32_t _reconnectMs;
  uint32_t _connectTimeoutMs;

  bool   _enabled    {false};
  bool   _connecting {false};
  bool   _connected  {false};
  String _ip;

  ms_t _lastAttempt  {0};
  ms_t _attemptStart {0};

  WiFiCb _onConnected    {nullptr};
  WiFiCb _onDisconnected {nullptr};
  WiFiCb _onGotIP        {nullptr};

  void _publish(Runtime& rt, uint16_t type);
};

} // namespace atg

#endif // ATG_ENABLE_WIFI
#endif // ESP32 || ESP8266