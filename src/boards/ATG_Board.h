#pragma once
#include <Arduino.h>

namespace atg {

enum class BoardKind : uint8_t {
  Unknown = 0,
  Uno,
  Esp32,
};

inline BoardKind detectBoard() {
#if defined(ESP32)
  return BoardKind::Esp32;
#elif defined(ARDUINO_AVR_UNO)
  return BoardKind::Uno;
#else
  return BoardKind::Unknown;
#endif
}

inline const __FlashStringHelper* boardName() {
  BoardKind b = detectBoard();
  switch (b) {
    case BoardKind::Esp32: return F("ESP32");
    case BoardKind::Uno:   return F("UNO");
    default:               return F("UNKNOWN");
  }
}

} // namespace atg