#pragma once
#include <Arduino.h>
#include "ATG_Types.h"

namespace atg {

class Runtime; // forward

class Module {
public:
  virtual ~Module() {}

  // Unique name for debugging / registry
  virtual const __FlashStringHelper* name() const = 0;

  // Called once on boot
  virtual Result begin(Runtime& rt) = 0;

  // Called repeatedly in loop
  virtual void tick(Runtime& rt) = 0;
};

} // namespace atg