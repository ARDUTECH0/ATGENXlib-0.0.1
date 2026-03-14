#if ATG_SMARTHOME_ONLY
#include "ATG_Manifest.h"

namespace atg {

ManifestBuilder::ManifestBuilder() {}

void ManifestBuilder::setProject(const String& projectId, const String& name, const String& version) {
  _projectId = projectId; _projectName = name; _projectVersion = version;
}
void ManifestBuilder::setHub(const String& hubId, const String& hubName) {
  _hubId = hubId; _hubName = hubName;
}
void ManifestBuilder::setEndpoints(uint16_t httpPort, uint16_t wsPort, const String& wsPath, const String& manifestPath) {
  _httpPort = httpPort; _wsPort = wsPort; _wsPath = wsPath; _manifestPath = manifestPath;
}

String ManifestBuilder::build(const DeviceRegistry& reg) const {
  StaticJsonDocument<4096> doc;

  doc["product"] = "ATGenX";
  doc["library"] = "ATGENXlib";
  doc["lanOnly"] = true;

  auto project = doc.createNestedObject("project");
  project["id"] = _projectId;
  project["name"] = _projectName;
  project["version"] = _projectVersion;

  auto hub = doc.createNestedObject("hub");
  hub["id"] = _hubId;
  hub["name"] = _hubName;
  hub["ip"] = WiFi.localIP().toString();
  hub["http"] = _httpPort;
  hub["ws"] = _wsPort;
  hub["wsPath"] = _wsPath;
  hub["manifestPath"] = _manifestPath;

  auto devices = doc.createNestedArray("devices");
  auto ui = doc.createNestedArray("ui");

  for (size_t i = 0; i < reg.count(); i++) {
auto e = reg.at(i);
    if (!e) continue;

    // device entry
    auto d = devices.createNestedObject();
    d["id"] = e->dev->id();
    d["type"] = e->dev->type();

    auto caps = d.createNestedArray("cap");
    e->dev->getCapsJson(caps);

    // ui entry
    auto u = ui.createNestedObject();
    u["deviceId"] = e->dev->id();
    const char* w = "switch";
    if (e->widget.type == UiWidget::Indicator) w = "indicator";
    else if (e->widget.type == UiWidget::Gauge) w = "gauge";
    else if (e->widget.type == UiWidget::Button) w = "button";
    u["widget"] = w;
    u["label"] = e->widget.label;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace atg
#endif