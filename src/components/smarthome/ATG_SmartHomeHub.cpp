#if ATG_SMARTHOME_ONLY
#include "ATG_SmartHomeHub.h"
#include <cstdint>         // uint16_t
#include <cstddef>         // size_t
namespace atg {

SmartHomeHub* SmartHomeHub::_self = nullptr;

SmartHomeHub::SmartHomeHub() {}

void SmartHomeHub::setIdentity(const String& hubId, const String& name) {
  _hubId = hubId; _hubName = name;
  _manifest.setHub(_hubId, _hubName);
}

void SmartHomeHub::setProject(const String& projectId, const String& name, const String& version) {
  _manifest.setProject(projectId, name, version);
}

void SmartHomeHub::enableUdpDiscovery(uint16_t port) { _udpPort = port; }
void SmartHomeHub::enableHttp(uint16_t port, const String& manifestPath) {
  _httpPort = port; _manifestPath = manifestPath;
}
void SmartHomeHub::enableWs(uint16_t port) { _wsPort = port; }

DeviceRegistry& SmartHomeHub::registry() { return _reg; }

String SmartHomeHub::_manifestThunk() { return _self ? _self->buildManifest() : String("{}"); }
String SmartHomeHub::_tokenThunk() { return _self ? _self->_http.token() : String(""); }

String SmartHomeHub::buildManifest() {
  _manifest.setEndpoints(_httpPort ? _httpPort : 80, _wsPort ? _wsPort : 8080, _wsPath, _manifestPath);
  return _manifest.build(_reg);
}

bool SmartHomeHub::begin() {
  _self = this;

  // ✅ hook all registered devices for automatic state broadcast
  for (size_t i = 0; i < _reg.count(); i++) {
    auto e = _reg.at(i);
    if (e && e->dev) {
      e->dev->setStateChangedCallback(&SmartHomeHub::onDeviceStateChangedStatic, this);
      Serial.printf("[Hub] hook device: %s\n", e->dev->id());
    }
  }

  if (_httpPort) {
    _http.setManifestProvider(&_manifestThunk);
    _http.begin(_httpPort, _manifestPath);
  }

  if (_wsPort) {
    _ws.setRegistry(&_reg);
    _ws.setAuthTokenProvider(&_tokenThunk);
    _ws.begin(_wsPort);
  }

  if (_udpPort) {
    _udp.setIdentity(_hubId, _hubName);
    _udp.setEndpoints(_httpPort ? _httpPort : 80, _wsPort ? _wsPort : 8080, _manifestPath);
    _udp.begin(_udpPort);
  }

  return true;
}
void SmartHomeHub::onDeviceStateChangedStatic(const char* deviceId, void* user) {
  if (!user) return;
  static_cast<SmartHomeHub*>(user)->onDeviceStateChanged(deviceId);
}

void SmartHomeHub::onDeviceStateChanged(const char* deviceId) {
  Serial.printf("[Hub] state changed: %s\n", deviceId);

  IDevice* dev = registry().findById(String(deviceId));
  if (!dev) {
    Serial.println("[Hub] device not found");
    return;
  }

  StaticJsonDocument<256> doc;
  JsonObject st = doc.to<JsonObject>();
  dev->getStateJson(st);

  String dbg;
  serializeJson(doc, dbg);
  Serial.printf("[Hub] broadcasting: %s\n", dbg.c_str());

  _ws.broadcastState(deviceId, st);
}
void SmartHomeHub::loop() {
  if (_httpPort) _http.loop();
  if (_wsPort) _ws.loop();
  if (_udpPort) _udp.loop();

  // loop devices
  for (size_t i = 0; i < _reg.count(); i++) {
    auto e = _reg.at(i);
    if (e && e->dev) e->dev->loop();
  }
}

void SmartHomeHub::openPairing() { _http.openPairing(); }
bool SmartHomeHub::hasToken() const { return _http.hasToken(); }

} // namespace atg
#endif