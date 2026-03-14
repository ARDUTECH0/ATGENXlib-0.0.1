#pragma once
#include <Arduino.h>

#include "../../core/ATG_Config.h"
#include "../../core/ATG_Types.h"
#include "../../core/ATG_Time.h"
#include "../../core/ATG_Log.h"
#include "../../core/ATG_Module.h"
#include "../../core/ATG_Runtime.h"

// ── Platform guard ────────────────────────────────────────────────
// WiFiPortal requires ESP32 or ESP8266.
// On ESP8266, Preferences is replaced with LittleFS/EEPROM internally.
#if !defined(ESP32) && !defined(ESP8266)
  #warning "ATG_WiFiPortal: unsupported platform — portal will be a no-op stub"
#endif

// ── Platform-specific server type ────────────────────────────────
#if defined(ESP32)
  #include <WebServer.h>
  using PlatformWebServer = WebServer;
#elif defined(ESP8266)
  #include <ESP8266WebServer.h>
  using PlatformWebServer = ESP8266WebServer;
#endif

namespace atg {

using PortalCb = void (*)();

class WiFiPortal : public Module {
public:
  WiFiPortal(const char* productName     = "ATGenX",
             const char* apPass          = "12345678",
             uint32_t portalTimeoutMs    = 0,
             uint32_t connectTimeoutMs   = 15000);

  const __FlashStringHelper* name() const override { return F("ATG_WiFiPortal"); }

  Result begin(Runtime& rt) override;
  void   tick (Runtime& rt) override;

  // ── State ─────────────────────────────────────────────
  bool   isPortalRunning() const { return _portalRunning; }
  bool   isConnected()     const { return _connected;     }
  String ip()              const { return _ip;            }

  // ── Callbacks ─────────────────────────────────────────
  void onPortalStart (PortalCb cb) { _onPortalStart  = cb; }
  void onSaved       (PortalCb cb) { _onSaved        = cb; }
  void onConnected   (PortalCb cb) { _onConnected    = cb; }
  void onDisconnected(PortalCb cb) { _onDisconnected = cb; }

  // ── Actions ───────────────────────────────────────────
  void resetCredentials();
  void startPortal();
  void stopPortal();

private:
  const char* _productName;
  const char* _apPass;
  uint32_t    _portalTimeoutMs;
  uint32_t    _connectTimeoutMs;

  bool   _portalRunning {false};
  bool   _connected     {false};
  bool   _connecting    {false};
  String _ip;

  ms_t _portalStart  {0};
  ms_t _connectStart {0};

  PortalCb _onPortalStart  {nullptr};
  PortalCb _onSaved        {nullptr};
  PortalCb _onConnected    {nullptr};
  PortalCb _onDisconnected {nullptr};

  // ── Internals ─────────────────────────────────────────
  bool   loadCredentials (String& ssid, String& pass);
  void   saveCredentials (const String& ssid, const String& pass);
  void   tryConnect      (const String& ssid, const String& pass);
  void   setupServer     ();
  String buildScanJson   ();

  // ── UI ────────────────────────────────────────────────
  String uiIndexHtml() const;
};

} // namespace atg