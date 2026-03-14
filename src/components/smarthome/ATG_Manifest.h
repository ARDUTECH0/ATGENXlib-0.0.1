#if ATG_SMARTHOME_ONLY

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "ATG_DeviceRegistry.h"

namespace atg {

class ManifestBuilder {
public:
  ManifestBuilder();

  void setProject(const String& projectId, const String& name, const String& version);
  void setHub(const String& hubId, const String& hubName);
  void setEndpoints(uint16_t httpPort, uint16_t wsPort, const String& wsPath, const String& manifestPath);

  // Builds JSON string (compact)
  String build(const DeviceRegistry& reg) const;

private:
  String _projectId = "p1";
  String _projectName = "ATGenX Home";
  String _projectVersion = "1.0";

  String _hubId = "hub-1";
  String _hubName = "ATGenX Hub";

  uint16_t _httpPort = 80;
  uint16_t _wsPort = 8080;
  String _wsPath = "/ws";
  String _manifestPath = "/atgenx/manifest";
};

} // namespace atg
#endif