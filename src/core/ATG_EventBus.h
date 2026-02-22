#pragma once
#include <Arduino.h>
#include "ATG_Config.h"
#include "ATG_Types.h"

namespace atg {

struct Event {
  u16 type;      // user-defined event type
  u32 a;         // generic payload
  u32 b;         // generic payload
};

using EventHandlerFn = void (*)(const Event& e);

class EventBus {
public:
  EventBus() : _subCount(0), _qHead(0), _qTail(0), _qCount(0) {}

  Result subscribe(u16 type, EventHandlerFn fn) {
    if (!fn) return Result::InvalidArg;
    if (_subCount >= ATG_MAX_SUBSCRIBERS) return Result::NoMemory;
    _subs[_subCount++] = {type, fn};
    return Result::Ok;
  }

  Result publish(const Event& e) {
    if (_qCount >= ATG_MAX_EVENTS_QUEUE) return Result::Busy;
    _queue[_qTail] = e;
    _qTail = (u8)((_qTail + 1) % ATG_MAX_EVENTS_QUEUE);
    _qCount++;
    return Result::Ok;
  }

  void pump() {
    // Dispatch queued events to subscribers
    while (_qCount > 0) {
      Event e = _queue[_qHead];
      _qHead = (u8)((_qHead + 1) % ATG_MAX_EVENTS_QUEUE);
      _qCount--;

      for (u8 i=0;i<_subCount;i++) {
        if (_subs[i].type == e.type) {
          _subs[i].fn(e);
        }
      }
    }
  }

private:
  struct Sub { u16 type; EventHandlerFn fn; };

  Sub _subs[ATG_MAX_SUBSCRIBERS];
  u8 _subCount;

  Event _queue[ATG_MAX_EVENTS_QUEUE];
  u8 _qHead, _qTail, _qCount;
};

} // namespace atg