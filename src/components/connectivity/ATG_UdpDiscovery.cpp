#if defined(ESP32) || defined(ESP8266)

#include "ATG_UdpDiscovery.h"

// ─────────────────────────────────────────────
// Platform-specific WiFi IP getter
// ESP8266 uses WiFi.localIP() same as ESP32,
// but we centralise it here for clarity.
// ─────────────────────────────────────────────
static inline String _localIp() {
  return WiFi.localIP().toString();
}

// ─────────────────────────────────────────────
// Tunables
// ─────────────────────────────────────────────
static constexpr uint16_t kRxBufSize  = 384;  // max incoming packet bytes
static constexpr uint16_t kTxBufSize  = 512;  // max HERE response bytes
static constexpr uint32_t kCooldownMs = 500;  // min ms between replies

namespace atg {

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
UdpDiscovery::UdpDiscovery() {}

// ─────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────
void UdpDiscovery::setIdentity(const String& hubId, const String& name) {
  _hubId = hubId;
  _name  = name;
}

void UdpDiscovery::setEndpoints(uint16_t httpPort, uint16_t wsPort,
                                const String& manifestPath) {
  _httpPort     = httpPort;
  _wsPort       = wsPort;
  _manifestPath = manifestPath;
}

// ─────────────────────────────────────────────
// begin()
// ─────────────────────────────────────────────
bool UdpDiscovery::begin(uint16_t port) {
  _port    = port;
  _running = (_udp.begin(_port) == 1);

  Serial.printf("[UDP][%s] Discovery %s on port %u\n",
    platformName(),
    _running ? "listening" : "FAILED",
    _port
  );

  return _running;
}

// ─────────────────────────────────────────────
// isDiscover()
// Validates the DISCOVER packet without a JSON
// parser — fast and safe on constrained MCUs.
// ─────────────────────────────────────────────
bool UdpDiscovery::isDiscover(const char* buf, int len) const {
  if (len < 10)                              return false;
  if (!strstr(buf, "\"t\":\"DISCOVER\""))   return false;
  if (!strstr(buf, "\"app\":\"atgenx\""))   return false;
  if (!strstr(buf, "\"v\":"))               return false;
  return true;
}

// ─────────────────────────────────────────────
// replyHere()
// Builds the HERE JSON into a stack buffer
// (avoids String heap fragmentation on ESP8266
// and ESP32 alike).
// ─────────────────────────────────────────────
void UdpDiscovery::replyHere(IPAddress remoteIp, uint16_t remotePort) {
  const String ip = _localIp();

  char resp[kTxBufSize];
  int written = snprintf(resp, sizeof(resp),
    "{"
      "\"t\":\"HERE\","
      "\"product\":\"ATGenX\","
      "\"platform\":\"%s\","
      "\"hubId\":\"%s\","
      "\"name\":\"%s\","
      "\"ip\":\"%s\","
      "\"http\":%u,"
      "\"ws\":%u,"
      "\"manifest\":\"http://%s%s\""
    "}",
    platformName(),
    _hubId.c_str(),
    _name.c_str(),
    ip.c_str(),
    _httpPort,
    _wsPort,
    ip.c_str(),
    _manifestPath.c_str()
  );

  if (written < 0 || written >= (int)sizeof(resp)) {
    Serial.println("[UDP] ERROR: HERE response truncated — check field lengths");
    return;
  }

  _udp.beginPacket(remoteIp, remotePort);
  _udp.write((const uint8_t*)resp, (size_t)written);
  _udp.endPacket();

  _replyCount++;
  Serial.printf("[UDP] HERE → %s:%u  (#%lu)\n",
    remoteIp.toString().c_str(),
    remotePort,
    (unsigned long)_replyCount
  );
}

// ─────────────────────────────────────────────
// loop() — call every iteration
// ─────────────────────────────────────────────
void UdpDiscovery::loop() {
  if (!_running) return;

  int sz = _udp.parsePacket();
  if (sz <= 0) return;

  // ── Capture sender address BEFORE read() ──
  // Some WiFiUdp implementations (ESP8266 in
  // particular) clear remoteIP after read().
  const IPAddress remoteIp   = _udp.remoteIP();
  const uint16_t  remotePort = _udp.remotePort();

  char buf[kRxBufSize];
  int  len = _udp.read(buf, sizeof(buf) - 1);
  if (len <= 0) return;
  buf[len] = '\0';

  if (!isDiscover(buf, len)) return;

  // ── Cooldown: ignore burst duplicates ─────
  const uint32_t now = millis();
  if ((now - _lastReplyMs) < kCooldownMs) {
    Serial.println("[UDP] DISCOVER ignored (cooldown)");
    return;
  }
  _lastReplyMs = now;

  replyHere(remoteIp, remotePort);
}

} // namespace atg
#endif // ESP32 || ESP8266