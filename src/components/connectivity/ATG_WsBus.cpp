#if defined(ESP32) || defined(ESP8266)

#include "ATG_WsBus.h"

// ESP8266 has ~80 KB heap — keep JSON docs smaller
#if defined(ESP8266)
  static constexpr size_t kRxDocSize  = 256;
  static constexpr size_t kAckDocSize = 96;
  static constexpr size_t kTxDocSize  = 256;
#else
  static constexpr size_t kRxDocSize  = 512;
  static constexpr size_t kAckDocSize = 128;
  static constexpr size_t kTxDocSize  = 384;
#endif

namespace atg {

// ─────────────────────────────────────────────────────────────────
// begin
// ─────────────────────────────────────────────────────────────────
bool WsBus::begin(uint16_t port) {
  _port = port;

  // Clean up any previous instance
  if (_ws) {
    _ws->close();
    delete _ws;
    _ws = nullptr;
  }

  _ws = new WebSocketsServer(_port);
  _ws->begin();

  Serial.printf("[WsBus] listening on port %u\n", _port);

  _ws->onEvent([this](uint8_t n, WStype_t t, uint8_t* p, size_t l) {
    this->onEvent(n, t, p, l);
  });

  return true;
}

// ─────────────────────────────────────────────────────────────────
// loop
// ─────────────────────────────────────────────────────────────────
void WsBus::loop() {
  if (_ws) _ws->loop();
}

// ─────────────────────────────────────────────────────────────────
// checkToken
// ─────────────────────────────────────────────────────────────────
bool WsBus::checkToken(JsonObject root) {
  if (!_getToken) return true;
  const String expected = _getToken();
  if (expected.length() == 0) return true;
  const char* tk = root["token"] | "";
  return expected == String(tk);
}

// ─────────────────────────────────────────────────────────────────
// onEvent
// ─────────────────────────────────────────────────────────────────
void WsBus::onEvent(uint8_t num, WStype_t type,
                    uint8_t* payload, size_t len) {
  switch (type) {
   case WStype_CONNECTED: 
  Serial.printf("[WsBus] client #%u connected\n", num);

  if (_reg && _ws) {
    StaticJsonDocument<kTxDocSize> snap;
    snap["t"] = "SNAPSHOT";
    JsonArray devs = snap.createNestedArray("devices");

    for (size_t i = 0; i < _reg->count(); i++) {
      auto e = _reg->at(i);
      if (!e || !e->dev) continue;

      JsonObject item = devs.createNestedObject();
      item["id"] = e->dev->id();

      JsonObject st = item.createNestedObject("state");
      e->dev->getStateJson(st);
    }

    String s;
    serializeJson(snap, s);
    _ws->sendTXT(num, s);
  }

  return;


    case WStype_DISCONNECTED:
      Serial.printf("[WsBus] client #%u disconnected\n", num);
      return;

    case WStype_TEXT:
      Serial.printf("[WsBus] RX from #%u: %.*s\n", num, (int)len, (const char*)payload);

      break;  

    default:
      return;
  }

  if (!_reg || !_ws) return;

  // ── Parse incoming JSON ────────────────────────────────
  StaticJsonDocument<kRxDocSize> doc;
  const DeserializationError err = deserializeJson(doc, payload, len);
  if (err) {
    Serial.printf("[WsBus] JSON parse error: %s\n", err.c_str());
    return;
  }

  JsonObject root = doc.as<JsonObject>();

  // ── Auth ──────────────────────────────────────────────
  if (!checkToken(root)) {
    _ws->sendTXT(num, "{\"error\":\"unauthorized\"}");
    return;
  }

  // ── Route to device ───────────────────────────────────
  const char* to = root["to"] | "";
  if (!to[0]) return;

  IDevice* dev = _reg->findById(String(to));
  if (!dev) {
    _ws->sendTXT(num, "{\"error\":\"unknown_device\"}");
    return;
  }

  bool handled = false;
  dev->handleCmdJson(root, handled);

  // ── ACK ───────────────────────────────────────────────
    // ── ACK + STATE update ────────────────────────────────
  if (handled) {
    // 1) ACK
    StaticJsonDocument<kAckDocSize> ack;
    ack["t"]   = "ACK";
    ack["to"]  = to;
    ack["seq"] = root["seq"] | 0;

    String ackStr;
    serializeJson(ack, ackStr);
    _ws->sendTXT(num, ackStr);

    // 2) STATE (broadcast to all clients)
    StaticJsonDocument<kTxDocSize> msg;
    msg["t"]    = "STATE";
    msg["from"] = to;

    JsonObject st = msg.createNestedObject("state");
    dev->getStateJson(st);     // ✅ خُد الحالة من الديفايس نفسه

    String stateStr;
    serializeJson(msg, stateStr);
    _ws->broadcastTXT(stateStr);
  }
}

// ─────────────────────────────────────────────────────────────────
// broadcastState
// ─────────────────────────────────────────────────────────────────
void WsBus::broadcastState(const char* deviceId, JsonObject state) {
  if (!_ws) return;

  StaticJsonDocument<kTxDocSize> msg;
  msg["t"]    = "STATE";
  msg["from"] = deviceId;

  JsonObject st = msg.createNestedObject("state");
  st.set(state); // ✅ copy object content safely

  String s;
  serializeJson(msg, s);
  _ws->broadcastTXT(s);
}

} // namespace atg

#endif // ESP32 || ESP8266