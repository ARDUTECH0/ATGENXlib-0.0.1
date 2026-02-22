#pragma once
#include <Arduino.h>
#include "ATG_Types.h"

namespace atg {

using StateId = u16;
using StateFn = void (*)();

struct StateDef {
  StateId id;
  const __FlashStringHelper* name;
  StateFn onEnter;
  StateFn onTick;
  StateFn onExit;
};

class StateMachine {
public:
  StateMachine() : _count(0), _current(0), _hasCurrent(false) {}

  Result add(StateId id, const __FlashStringHelper* name, StateFn onEnter=nullptr, StateFn onTick=nullptr, StateFn onExit=nullptr) {
    if (_count >= 16) return Result::NoMemory;
    _states[_count++] = {id, name, onEnter, onTick, onExit};
    return Result::Ok;
  }

  Result set(StateId id) {
    StateDef* next = find(id);
    if (!next) return Result::NotFound;

    if (_hasCurrent && _current->onExit) _current->onExit();
    _current = next;
    _hasCurrent = true;
    if (_current->onEnter) _current->onEnter();
    return Result::Ok;
  }

  void tick() {
    if (_hasCurrent && _current->onTick) _current->onTick();
  }

  StateId current() const { return _hasCurrent ? _current->id : 0; }
  const __FlashStringHelper* currentName() const { return _hasCurrent ? _current->name : F("NONE"); }

private:
  StateDef* find(StateId id) {
    for (u8 i=0;i<_count;i++) if (_states[i].id == id) return &_states[i];
    return nullptr;
  }

  StateDef _states[16];
  u8 _count;

  StateDef* _current;
  bool _hasCurrent;
};

} // namespace atg