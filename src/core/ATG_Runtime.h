#pragma once
#include <Arduino.h>
#include "ATG_Config.h"
#include "ATG_Types.h"
#include "ATG_Log.h"
#include "ATG_EventBus.h"
#include "ATG_Scheduler.h"
#include "ATG_Module.h"

namespace atg {

class Runtime {
public:
  Runtime() : _modCount(0), _booted(false) {}

  void attachLogger(LogSink* sink, LogLevel lvl = (LogLevel)ATG_LOG_LEVEL) {
    Log::setSink(sink);
    Log::setLevel(lvl);
  }

  EventBus& events() { return _bus; }
  Scheduler& scheduler() { return _sched; }

  Result addModule(Module& m) {
    if (_modCount >= ATG_MAX_MODULES) return Result::NoMemory;
    _mods[_modCount++] = &m;
    return Result::Ok;
  }

  Result begin() {
    if (_booted) return Result::Busy;
    _booted = true;

    for (u8 i=0;i<_modCount;i++) {
      Module* m = _mods[i];
      Result r = m->begin(*this);
      if (r != Result::Ok) {
        Log::e(F("ATG"), String("Module begin failed: ") + (const __FlashStringHelper*)m->name());
        return r;
      }
      Log::i(F("ATG"), String("Module ready: ") + (const __FlashStringHelper*)m->name());
    }
    return Result::Ok;
  }

  void loopOnce() {
    // 1) scheduler tasks
    _sched.tick();

    // 2) module ticks
    for (u8 i=0;i<_modCount;i++) {
      _mods[i]->tick(*this);
    }

    // 3) events dispatch
    _bus.pump();
  }

private:
  Module* _mods[ATG_MAX_MODULES];
  u8 _modCount;
  bool _booted;

  EventBus _bus;
  Scheduler _sched;
};

} // namespace atg