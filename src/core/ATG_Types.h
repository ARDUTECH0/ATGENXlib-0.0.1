#pragma once
#include <Arduino.h>

namespace atg {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using ms_t = uint32_t;

enum class Result : uint8_t {
  Ok = 0,
  Error,
  Busy,
  Timeout,
  NotFound,
  InvalidArg,
  NoMemory,
};

struct SpanBytes {
  const uint8_t* data;
  size_t size;
};

} // namespace atg