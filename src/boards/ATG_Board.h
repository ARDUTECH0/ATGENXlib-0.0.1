#pragma once
#include <Arduino.h>

namespace atg {

enum class BoardKind : uint8_t {
  Unknown = 0,
  Uno,
  Nano,
  Mega,
  Esp32,
  Esp8266
};

inline BoardKind detectBoard() {

#if defined(ESP32)
  return BoardKind::Esp32;

#elif defined(ESP8266)
  return BoardKind::Esp8266;

#elif defined(ARDUINO_AVR_MEGA2560)
  return BoardKind::Mega;

#elif defined(ARDUINO_AVR_UNO)
  return BoardKind::Uno;

#elif defined(ARDUINO_AVR_NANO)
  return BoardKind::Nano;

#else
  return BoardKind::Unknown;
#endif

}

inline const __FlashStringHelper* boardName() {
  switch (detectBoard()) {
    case BoardKind::Esp32:   return F("ESP32");
    case BoardKind::Esp8266: return F("ESP8266");
    case BoardKind::Mega:    return F("MEGA2560");
    case BoardKind::Uno:     return F("UNO");
    case BoardKind::Nano:    return F("NANO");
    default:                 return F("UNKNOWN");
  }
}

constexpr bool isWiFiSupported() {

#if defined(ESP32)
  return true;

#elif defined(ESP8266)
  return true;

#else
  return false;

#endif

}
} // namespace atg