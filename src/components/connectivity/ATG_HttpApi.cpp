#if defined(ESP32)
#include "ATG_HttpApi.h"
#include <ArduinoJson.h>

namespace atg {

HttpApi::HttpApi() {}

void HttpApi::setManifestProvider(ManifestFn fn) { _manifestFn = fn; }
void HttpApi::setPairingWindowMs(uint32_t ms) { _pairWindowMs = ms; }

void HttpApi::loadToken() {
  _prefs.begin("atgenx", true);
  _token = _prefs.getString("token", "");
  _prefs.end();
}

void HttpApi::saveToken(const String& t) {
  _prefs.begin("atgenx", false);
  _prefs.putString("token", t);
  _prefs.end();
  _token = t;
}

bool HttpApi::begin(uint16_t port, const String& manifestPath) {
  _manifestPath = manifestPath;
  loadToken();

  _port = port;

  if (_server) {
    delete _server;
    _server = nullptr;
  }
  _server = new WebServer(_port);

  _server->on(_manifestPath.c_str(), HTTP_GET, [this]() { handleManifest(); });
  _server->on("/atgenx/pair", HTTP_POST, [this]() { handlePair(); });

  _server->onNotFound([this]() {
    _server->send(404, "application/json", "{\"error\":\"not_found\"}");
  });

  _server->begin();
  return true;
}

void HttpApi::loop() {
  if (_server) _server->handleClient();
}

void HttpApi::openPairing() { _pairDeadline = millis() + _pairWindowMs; }
void HttpApi::closePairing() { _pairDeadline = 0; }

bool HttpApi::hasToken() const { return _token.length() > 0; }
String HttpApi::token() const { return _token; }

void HttpApi::handleManifest() {
  if (!_server) return;

  if (!_manifestFn) {
    _server->send(500, "application/json", "{\"error\":\"manifest_provider_missing\"}");
    return;
  }

  _server->sendHeader("Access-Control-Allow-Origin", "*");
  _server->send(200, "application/json", _manifestFn());
}

void HttpApi::handlePair() {
  if (!_server) return;

  // Allow pairing only in window OR if no token exists yet
  const bool allow = (!hasToken()) || (_pairDeadline != 0 && (int32_t)(millis() - _pairDeadline) < 0);
  if (!allow) {
    _server->send(403, "application/json", "{\"error\":\"pairing_closed\"}");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, _server->arg("plain"));
  if (err) {
    _server->send(400, "application/json", "{\"error\":\"bad_json\"}");
    return;
  }

  const char* pairCode = doc["pairCode"] | "";
  if (strlen(pairCode) < 4) {
    _server->send(400, "application/json", "{\"error\":\"bad_pair_code\"}");
    return;
  }

  // Generate token (simple). Replace with stronger random if you want.
  String t = "tk_";
  t += String((uint32_t)esp_random(), HEX);
  t += String((uint32_t)esp_random(), HEX);

  saveToken(t);

  StaticJsonDocument<256> out;
  out["ok"] = true;
  out["token"] = t;
  String s;
  serializeJson(out, s);

  _server->sendHeader("Access-Control-Allow-Origin", "*");
  _server->send(200, "application/json", s);

  closePairing();
}

} // namespace atg
#endif