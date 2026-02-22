#pragma once

// =========================
// ATGENXlib Config
// =========================

// 0 = off, 1 = errors, 2 = info, 3 = debug
#ifndef ATG_LOG_LEVEL
#define ATG_LOG_LEVEL 2
#endif

#ifndef ATG_MAX_MODULES
#define ATG_MAX_MODULES 24
#endif

#ifndef ATG_MAX_SUBSCRIBERS
#define ATG_MAX_SUBSCRIBERS 32
#endif

#ifndef ATG_MAX_EVENTS_QUEUE
#define ATG_MAX_EVENTS_QUEUE 16
#endif

#ifndef ATG_MAX_TASKS
#define ATG_MAX_TASKS 16
#endif

#ifndef ATG_ENABLE_ASSERT
#define ATG_ENABLE_ASSERT 1
#endif

// millis() wrap safe comparisons handled internally