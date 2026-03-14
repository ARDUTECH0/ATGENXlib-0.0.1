#pragma once
#if ATG_SMARTHOME_ONLY

#if defined(ESP32) || defined(ESP8266)  
#include <Arduino.h>

#include "ATG_DeviceRegistry.h"
#include "ATG_Manifest.h"

#include "../connectivity/ATG_UdpDiscovery.h"
#include "../connectivity/ATG_HttpApi.h"
#include "../connectivity/ATG_WsBus.h"

namespace atg {

class SmartHomeHub {
public:
  SmartHomeHub();

  void setIdentity(const String& hubId, const String& name);
  void setProject(const String& projectId, const String& name, const String& version);

  // enable endpoints
  void enableUdpDiscovery(uint16_t port = 4210);
  void enableHttp(uint16_t port = 80, const String& manifestPath = "/atgenx/manifest");
  void enableWs(uint16_t port = 8080);

  DeviceRegistry& registry();

  bool begin();
  void loop();

  // Pairing UX helpers (call from Serial / button)
  void openPairing();
  bool hasToken() const;

private:
  String _hubId = "hub-1";
  String _hubName = "ATGenX Hub";

  uint16_t _udpPort = 0;
  uint16_t _httpPort = 0;
  uint16_t _wsPort = 0;
  String _manifestPath = "/atgenx/manifest";
  String _wsPath = "/ws"; // reserved for future async mode

  DeviceRegistry _reg;
  ManifestBuilder _manifest;
  static void onDeviceStateChangedStatic(const char* deviceId, void* user);
  void onDeviceStateChanged(const char* deviceId);
  UdpDiscovery _udp;
  HttpApi _http;
  WsBus _ws;

  static SmartHomeHub* _self;
  static String _manifestThunk();
  static String _tokenThunk();

  String buildManifest();
};

} // namespace atg
#endif
#endif