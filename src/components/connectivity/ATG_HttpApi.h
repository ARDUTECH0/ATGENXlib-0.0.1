#pragma once
#if defined(ESP32)
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>

namespace atg {

class HttpApi {
public:
  HttpApi();

  using ManifestFn = String (*)();

  void setManifestProvider(ManifestFn fn);
  void setPairingWindowMs(uint32_t ms);

  bool begin(uint16_t port, const String& manifestPath = "/atgenx/manifest");
  void loop();

  void openPairing();
  void closePairing();
  bool hasToken() const;
  String token() const;

private:
  WebServer* _server = nullptr;   // ✅ pointer
  uint16_t _port = 80;

  Preferences _prefs;

  ManifestFn _manifestFn = nullptr;
  String _manifestPath = "/atgenx/manifest";

  uint32_t _pairWindowMs = 60000;
  uint32_t _pairDeadline = 0;

  String _token;

  void loadToken();
  void saveToken(const String& t);

  void handleManifest();
  void handlePair();
};

} // namespace atg
#endif