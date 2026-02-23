#pragma once
#include <Arduino.h>
#include "../../core/ATG_Module.h"
#include "../../core/ATG_Types.h"
#include "../../core/ATG_Log.h"
#include "../../core/ATG_Time.h"
#include "../../core/ATG_EventBus.h"
#include "../../core/ATG_Config.h"

namespace atg {

// Events (اختياري تستخدمه في المنصة)
enum : uint16_t {
  EVT_WIFI_CONNECTED = 2001,
  EVT_WIFI_DISCONNECTED = 2002,
  EVT_WIFI_GOT_IP = 2003,
};

using WiFiCb = void (*)();

class WiFiModule : public Module {
public:
  WiFiModule(const char* ssid, const char* pass,
             uint32_t reconnectMs = 5000,
             uint32_t connectTimeoutMs = 15000);

  const __FlashStringHelper* name() const override { return F("ATG_WiFi"); }

  Result begin(Runtime& rt) override;
  void tick(Runtime& rt) override;

  // Actions
  Result connect();        // non-blocking: تبدأ محاولة الاتصال
  void disconnect();

  // Status
  bool isEnabled() const { return _enabled; }
  bool isConnected() const { return _connected; }
  String ip() const { return _ip; }

  // Callbacks
  void onConnected(WiFiCb cb) { _onConnected = cb; }
  void onDisconnected(WiFiCb cb) { _onDisconnected = cb; }
  void onGotIP(WiFiCb cb) { _onGotIP = cb; }

  // Optional: update credentials
  void setCredentials(const char* ssid, const char* pass);

private:
  bool _enabled{false};

  const char* _ssid;
  const char* _pass;

  uint32_t _reconnectMs;
  uint32_t _connectTimeoutMs;

  ms_t _lastAttempt{0};
  ms_t _attemptStart{0};

  bool _connecting{false};
  bool _connected{false};
  String _ip;

  WiFiCb _onConnected{nullptr};
  WiFiCb _onDisconnected{nullptr};
  WiFiCb _onGotIP{nullptr};

  void publish(Runtime& rt, uint16_t type);
};

} // namespace atg