#pragma once

// ─────────────────────────────────────────────
// Platform guard — ESP32 (all variants) + ESP8266
// ESP32-C3 is detected via CONFIG_IDF_TARGET_ESP32C3
// or simply falls under the ESP32 umbrella define.
// ─────────────────────────────────────────────
#if defined(ESP32) || defined(ESP8266)

#include <Arduino.h>
#include <WiFiUdp.h>

// Platform-specific WiFi header
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

namespace atg {

class UdpDiscovery {
public:
  UdpDiscovery();

  // ── Configuration ─────────────────────────
  void setIdentity(const String& hubId, const String& name);
  void setEndpoints(uint16_t httpPort, uint16_t wsPort,
                    const String& manifestPath);

  // ── Lifecycle ─────────────────────────────
  // Call once in setup() after WiFi is connected
  bool begin(uint16_t port = 4210);

  // Call every loop() iteration
  void loop();

  // ── Status ────────────────────────────────
  bool     isRunning()   const { return _running; }
  uint32_t replyCount()  const { return _replyCount; }

  // ── Platform info (for debugging) ─────────
  static const char* platformName() {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
    return "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    return "ESP32-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    return "ESP32-S3";
#elif defined(ESP32)
    return "ESP32";
#elif defined(ESP8266)
    return "ESP8266";
#else
    return "Unknown";
#endif
  }

private:
  // ── Helpers ───────────────────────────────
  bool isDiscover(const char* buf, int len) const;
  void replyHere(IPAddress remoteIp, uint16_t remotePort);

  // ── State ─────────────────────────────────
  WiFiUDP  _udp;
  uint16_t _port        = 4210;
  bool     _running     = false;
  uint32_t _replyCount  = 0;
  uint32_t _lastReplyMs = 0;

  // ── Identity ──────────────────────────────
  String   _hubId;
  String   _name;
  uint16_t _httpPort     = 80;
  uint16_t _wsPort       = 8080;
  String   _manifestPath = "/atgenx/manifest";
};

} // namespace atg
#endif // ESP32 || ESP8266