#pragma once
#include <Arduino.h>
#include "ATG_Config.h"

namespace atg {

enum class LogLevel : uint8_t { Error=1, Info=2, Debug=3 };

class LogSink {
public:
  virtual ~LogSink() {}
  virtual void write(LogLevel lvl, const __FlashStringHelper* tag, const String& msg) = 0;
};

class SerialLogSink : public LogSink {
public:
  explicit SerialLogSink(Stream& s) : _s(s) {}
  void write(LogLevel lvl, const __FlashStringHelper* tag, const String& msg) override {
    // [I] TAG: message
    char c = (lvl==LogLevel::Error?'E':(lvl==LogLevel::Info?'I':'D'));
    _s.print('['); _s.print(c); _s.print("] ");
    _s.print(tag); _s.print(": ");
    _s.println(msg);
  }
private:
  Stream& _s;
};

class Log {
public:
  static void setSink(LogSink* sink) { _sink = sink; }
  static void setLevel(LogLevel lvl) { _level = lvl; }
  static LogLevel level() { return _level; }

  static void e(const __FlashStringHelper* tag, const String& msg) { out(LogLevel::Error, tag, msg); }
  static void i(const __FlashStringHelper* tag, const String& msg) { out(LogLevel::Info,  tag, msg); }
  static void d(const __FlashStringHelper* tag, const String& msg) { out(LogLevel::Debug, tag, msg); }

private:
  static void out(LogLevel lvl, const __FlashStringHelper* tag, const String& msg) {
#if ATG_LOG_LEVEL == 0
    (void)lvl; (void)tag; (void)msg;
#else
    if ((uint8_t)lvl > (uint8_t)_level) return;
    if (_sink) _sink->write(lvl, tag, msg);
#endif
  }

  static LogSink* _sink;
  static LogLevel _level;
};

} // namespace atg