#pragma once
#include <Arduino.h>
#include "ATG_Config.h"
#include "ATG_Types.h"
#include "ATG_Time.h"

namespace atg {

using TaskFn = void (*)();

struct Task {
  TaskFn fn;
  ms_t interval;
  ms_t last;
  bool enabled;
  bool runImmediately;
};

class Scheduler {
public:
  Scheduler() : _count(0) {}

  Result every(ms_t interval, TaskFn fn, bool runImmediately=false) {
    if (!fn || interval == 0) return Result::InvalidArg;
    if (_count >= ATG_MAX_TASKS) return Result::NoMemory;

    _tasks[_count++] = {fn, interval, nowMs(), true, runImmediately};
    return Result::Ok;
  }

  void enableAll(bool en) {
    for (u8 i=0;i<_count;i++) _tasks[i].enabled = en;
  }

  void tick() {
    const ms_t t = nowMs();
    for (u8 i=0;i<_count;i++) {
      Task& task = _tasks[i];
      if (!task.enabled) continue;

      if (task.runImmediately) {
        task.runImmediately = false;
        task.last = t;
        task.fn();
        continue;
      }

      if ((ms_t)(t - task.last) >= task.interval) {
        task.last = t;
        task.fn();
      }
    }
  }

private:
  Task _tasks[ATG_MAX_TASKS];
  u8 _count;
};

} // namespace atg