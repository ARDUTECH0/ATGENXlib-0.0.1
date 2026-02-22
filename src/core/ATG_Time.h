#pragma once
#include <Arduino.h>
#include "ATG_Types.h"

namespace atg {

inline ms_t nowMs() { return (ms_t)millis(); }

// Wrap-safe elapsed check
inline bool elapsed(ms_t start, ms_t interval) {
  return (ms_t)(nowMs() - start) >= interval;
}

} // namespace atg