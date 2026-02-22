#include "ATG_Log.h"
namespace atg {
LogSink* Log::_sink = nullptr;
LogLevel Log::_level = (LogLevel)ATG_LOG_LEVEL;
}