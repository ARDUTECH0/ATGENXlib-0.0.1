#pragma once
/**
 * ATG_Phone — text messages and calls from a SIM board (LILYGO T-A7670X, T-SIM7600X, T-SIM7670G-S3,
 * T-SIM7080G-S3, TTGO T-Call), made dependable:
 *
 *  - One at a time, from a queue: a rule that fires while another message is going waits its turn.
 *  - A message the network refuses is tried again (3 tries, 20 s apart); with no network yet it waits for one.
 *  - The board's internet is paused around them (onBusy). On these modems a message sent in the middle of the
 *    cloud connection drops it, and a call on a network without 4G voice moves the modem to 2G, which suspends
 *    the data anyway. So: the cloud is closed, the messages and calls go, the cloud comes back by itself.
 *  - Calls ring 30 s each, one number after the other, while loop() keeps running.
 *  - Arabic messages (70 letters a message; a longer text goes as several).
 *  - The board answers a message: "STATUS", "حالة" or "?" from one of its own numbers gets the readings back,
 *    and words of your own get what you choose: phone.reply("حرارة", temperatureText) — "حرارة" gets the
 *    temperature. Arabic is matched loosely (ال, ة/ه, أ/إ/آ, ى/ي, ?), English in any case, inside a longer
 *    message too ("الحرارة كام؟").
 *  - With no internet at all (messages and calls only) it works the same: onBusy is simply not set.
 *
 * Include it after TinyGsmClient.h, the modem chosen first:
 *
 *   #define TINY_GSM_MODEM_A7670
 *   #include <TinyGsmClient.h>
 *   #include <ATG_Phone.h>
 *
 *   TinyGsm modem(Serial1);
 *   ATG_Phone phone(modem);
 *   ATG_PhoneAlert hot(phone, "01012345678", "", "", "Too hot", 5 * 60000UL, true, true);
 *
 *   void setup() { ... modem.init() ...; phone.begin(); }
 *   void loop() { phone.loop(); if (temperature > 40) hot.turnOn(temperature); else hot.turnOff(); }
 *
 * Define ATG_PHONE_NO_CALLS before including it for a modem that has no voice (SIM7080G).
 */

#include <Arduino.h>
#include <math.h>

#ifndef SRC_TINYGSMCLIENT_H_
#error "ATG_Phone: #include <TinyGsmClient.h> (after #define TINY_GSM_MODEM_...) before <ATG_Phone.h>"
#endif

class ATG_Phone;

/**
 * One alert a rule switches on: a message, a call or both, to up to three numbers — once each time it is switched
 * on, then not again before the gap. turnOn(value) puts the reading at the end of the message: "Too hot (41.5)".
 */
class ATG_PhoneAlert {
 public:
  ATG_PhoneAlert(ATG_Phone& phone, const char* a, const char* b, const char* c, const char* text, uint32_t gapMs,
                 bool sms = true, bool call = false)
      : _phone(phone), _text(text), _gap(gapMs), _sms(sms), _call(call) {
    _to[0] = a;
    _to[1] = b;
    _to[2] = c;
    _n = (c && *c) ? 3 : (b && *b) ? 2 : 1;
  }

  void begin() {}
  inline void turnOn(float value = NAN);
  void turnOff() { _on = false; }
  void toggle() {
    if (_on) turnOff();
    else turnOn();
  }
  bool isOn() const { return _on; }

  /** messages sent, calls placed, and those that failed after every try */
  uint16_t sent = 0, calls = 0, failed = 0;

  /** true once after a message or a call went (or failed): the moment to tell the cloud */
  bool changed() {
    const bool c = _changed;
    _changed = false;
    return c;
  }

  /** the phone tells it how each one went */
  void done(bool call, bool ok) {
    if (!ok) ++failed;
    else if (call) ++calls;
    else ++sent;
    _changed = true;
  }

  uint8_t numbers() const { return _n; }
  const char* number(uint8_t i) const { return i < _n ? _to[i] : ""; }

 private:
  ATG_Phone& _phone;
  const char* _to[3];
  uint8_t _n;
  const char* _text;
  uint32_t _gap;
  bool _sms, _call;
  bool _on = false, _went = false, _changed = false;
  uint32_t _at = 0;
};

class ATG_Phone {
 public:
  static constexpr uint8_t QUEUE = 10;
  static constexpr uint8_t TRIES = 3;
  static constexpr uint32_t RETRY_MS = 20000UL;
  static constexpr uint32_t RING_MS = 30000UL;
  /** how long a message waits for the network before it is given up (the SIM out of coverage) */
  static constexpr uint32_t NETWORK_WAIT_MS = 10UL * 60000UL;
  /** how many words of your own the board answers (reply()) */
  static constexpr uint8_t WORDS = 8;

  /** What writes an answer: the text the board sends back. */
  typedef String (*Answer)();

  explicit ATG_Phone(TinyGsm& modem) : _m(modem) {}

  /**
   * What pauses the board's internet before a message or a call, and what brings it back after (both optional).
   * With ATGenX_Hub over the SIM: pause closes the cloud connection (`simNet.stop()`), and the sketch's link check
   * says "down" while busy() so the hub waits; it reconnects by itself afterwards.
   */
  void onBusy(void (*pause)(), void (*resume)() = nullptr) {
    _pause = pause;
    _resume = resume;
  }

  /** After the modem answers (modem.init()): text mode, no message popping up on the line, calls over 4G first. */
  void begin() {
    _m.sendAT(GF("+CMGF=1"));
    _m.waitResponse();
    // new messages are kept on the SIM without a notice on the line (they'd land in the middle of the cloud
    // connection); poll reads them
    _m.sendAT(GF("+CNMI=0,0,0,0,0"));
    _m.waitResponse();
#ifndef ATG_PHONE_NO_CALLS
    // A76xx / SIM76xx: voice over 4G where the network has it, so a call doesn't suspend the data. Other modems
    // answer ERROR, which changes nothing.
    _m.sendAT(GF("+CEVDP=3"));
    _m.waitResponse(2000L);
#endif
    _ready = true;
  }

  /**
   * The numbers the board takes questions from, and the answer to "STATUS" / "حالة" / "?" (what status() writes:
   * the readings). status may be nullptr: then only the words given to reply() are answered.
   */
  void answer(const char* a, const char* b, const char* c, Answer status) {
    _allowed[0] = a;
    _allowed[1] = b;
    _allowed[2] = c;
    _status = status;
  }

  /**
   * A word of your own and its answer: phone.reply("حرارة", temperatureText). A message with that word, from one
   * of the numbers given to answer(), gets what fn() writes. Up to 8 words; false when they're full.
   */
  bool reply(const char* word, Answer fn) {
    if (!word || !*word || !fn || _nWords >= WORDS) return false;
    _words[_nWords] = word;
    _replies[_nWords] = fn;
    ++_nWords;
    return true;
  }

  /** Queue a text message (split in several when it's long); false when the queue is full. */
  bool sms(const char* to, const String& text, ATG_PhoneAlert* from = nullptr) {
    if (!to || !*to) return false;
    // a long text: several messages, cut at a line or a space
    const bool wide = !ascii(text);
    const size_t max = wide ? 70 : 160;
    String rest = text;
    bool ok = true;
    while (rest.length()) {
      size_t cut = cutAt(rest, max);
      String part = rest.substring(0, cut);
      rest = rest.substring(cut);
      rest.trim();
      ok = push(KIND_SMS, to, part, rest.length() ? nullptr : from) && ok;
    }
    return ok;
  }

  /** Queue a call (rings 30 s). */
  bool call(const char* to, ATG_PhoneAlert* from = nullptr) {
#ifdef ATG_PHONE_NO_CALLS
    (void)to;
    (void)from;
    return false;
#else
    if (!to || !*to) return false;
    return push(KIND_CALL, to, String(), from);
#endif
  }

  /** true while a message or a call is going (the internet is paused meanwhile) */
  bool busy() const { return _paused; }

  /** Messages waiting to go (and the call ringing). */
  uint8_t pending() const { return _count + (_ringing ? 1 : 0); }

  /** Call every loop(): sends what's queued, ends the ringing call, reads the messages that came in. */
  void loop() {
    if (!_ready) return;
    const uint32_t now = millis();

    // a call rings its 30 s, then hangs up
    if (_ringing) {
      if (now - _ringAt < RING_MS) return;
#ifndef ATG_PHONE_NO_CALLS
      _m.callHangup();
#endif
      _ringing = false;
      Serial.println(F("[SIM] Call ended"));
    }

    if (!_count) {
      if (_paused) {
        _paused = false;
        if (_pause) Serial.println(F("[SIM] Messages and calls done — internet back"));
        if (_resume) _resume();
      }
      if ((_status || _nWords) && now - _inboxAt >= (_pause ? 60000UL : 15000UL)) {
        _inboxAt = now;
        readInbox();
      }
      return;
    }

    Job& j = _q[_head];
    if ((int32_t)(now - j.at) < 0) return;  // a retry, or the internet just paused: not yet

    // pause the internet first, and give it half a second to close
    if (!_paused) {
      _paused = true;
      if (_pause) {
        Serial.println(F("[SIM] Pausing the internet for a message / call"));
        _pause();
        j.at = now + 500;
        return;
      }
    }

    if (!_m.isNetworkConnected()) {
      if (!j.waitFrom) j.waitFrom = now ? now : 1;
      if (now - j.waitFrom >= NETWORK_WAIT_MS) {
        Serial.print(F("[SIM] No network for 10 minutes — gave up on "));
        Serial.println(j.to);
        finish(false);
        return;
      }
      j.at = now + 5000;
      return;
    }

    if (j.kind == KIND_SMS) {
      const bool ok = send(j.to, j.text);
      Serial.print(F("[SIM] SMS to "));
      Serial.print(j.to);
      Serial.println(ok ? F(" sent") : F(" not sent"));
      if (ok) finish(true);
      else if (++j.tries >= TRIES) finish(false);
      else j.at = now + RETRY_MS;
      return;
    }

#ifndef ATG_PHONE_NO_CALLS
    Serial.print(F("[SIM] Calling "));
    Serial.println(j.to);
    if (_m.callNumber(j.to)) {
      _ringing = true;
      _ringAt = now;
      finish(true);
    } else if (++j.tries >= TRIES) finish(false);
    else j.at = now + RETRY_MS;
#else
    finish(false);
#endif
  }

  /** totals, for the serial monitor or the cloud */
  uint16_t sentTotal = 0, callsTotal = 0, failedTotal = 0;

 private:
  enum : uint8_t { KIND_SMS = 0, KIND_CALL = 1 };
  struct Job {
    uint8_t kind = KIND_SMS;
    char to[24] = "";
    String text;
    uint8_t tries = 0;
    uint32_t at = 0, waitFrom = 0;
    ATG_PhoneAlert* from = nullptr;
  };

  TinyGsm& _m;
  Job _q[QUEUE];
  uint8_t _head = 0, _count = 0;
  bool _ready = false, _paused = false, _ringing = false;
  uint32_t _ringAt = 0, _inboxAt = 0;
  /** when the last three answers went (a ring): three in three minutes at most */
  uint32_t _answers[3] = {0, 0, 0};
  uint8_t _answerAt = 0;
  void (*_pause)() = nullptr;
  void (*_resume)() = nullptr;
  const char* _allowed[3] = {"", "", ""};
  Answer _status = nullptr;
  const char* _words[WORDS] = {};
  Answer _replies[WORDS] = {};
  uint8_t _nWords = 0;

  bool push(uint8_t kind, const char* to, const String& text, ATG_PhoneAlert* from) {
    if (_count >= QUEUE) {
      Serial.println(F("[SIM] Too many messages waiting — this one is dropped"));
      if (from) from->done(kind == KIND_CALL, false);
      return false;
    }
    Job& j = _q[(_head + _count) % QUEUE];
    j = Job();
    j.kind = kind;
    strncpy(j.to, to, sizeof(j.to) - 1);
    j.text = text;
    j.at = millis();
    j.from = from;
    ++_count;
    return true;
  }

  void finish(bool ok) {
    Job& j = _q[_head];
    if (!ok) ++failedTotal;
    else if (j.kind == KIND_CALL) ++callsTotal;
    else ++sentTotal;
    if (j.from) j.from->done(j.kind == KIND_CALL, ok);
    j.text = String();
    _head = (_head + 1) % QUEUE;
    --_count;
  }

  static bool ascii(const String& s) {
    for (size_t i = 0; i < s.length(); ++i)
      if ((uint8_t)s[i] >= 0x80) return false;
    return true;
  }

  /** where to cut a text of at most `max` letters (UTF-16 units for Arabic): at a line, else a space */
  static size_t cutAt(const String& s, size_t max) {
    size_t letters = 0, i = 0, line = 0, space = 0;
    while (i < s.length()) {
      const uint8_t c = (uint8_t)s[i];
      const size_t w = c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
      if (letters + (w == 4 ? 2 : 1) > max) break;
      letters += w == 4 ? 2 : 1;
      if (c == '\n') line = i + 1;
      else if (c == ' ') space = i + 1;
      i += w;
    }
    if (i >= s.length()) return s.length();
    if (line > max / 3) return line;
    if (space > max / 3) return space;
    return i;
  }

  /** UTF-8 → UCS-2 in hex, as the modem takes it in UCS2 mode: "+20" → "002B00320030" */
  static String ucs2Hex(const String& s, size_t maxLetters) {
    static const char H[] = "0123456789ABCDEF";
    String out;
    size_t n = 0;
    const uint8_t* p = (const uint8_t*)s.c_str();
    while (*p && n < maxLetters) {
      uint16_t u;
      if (*p < 0x80) u = *p++;
      else if ((*p & 0xE0) == 0xC0 && p[1]) {
        u = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        p += 2;
      } else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) {
        u = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        p += 3;
      } else {
        ++p;
        continue;
      }
      out += H[(u >> 12) & 0xF];
      out += H[(u >> 8) & 0xF];
      out += H[(u >> 4) & 0xF];
      out += H[u & 0xF];
      ++n;
    }
    return out;
  }

  /**
   * A text message, sent here rather than through TinyGSM's sendSMS / sendSMS_UTF16: the latter asks for
   * AT+CSCS="HEX", which only the SIM800 has (the A76xx and SIM76xx answer ERROR and the Arabic message never
   * goes), and leaves the coding at UCS-2 (AT+CSMP …,8) so the next Latin message went garbled. Here: plain
   * letters in the GSM alphabet, Arabic in UCS2 mode (the number in UCS-2 too, as SIMCom's manuals say), and the
   * modem put back to GSM after each one. Every SIMCom modem here (SIM800, A76xx, SIM7600, SIM7670) takes both.
   */
  bool send(const char* to, const String& text) {
    const bool wide = !ascii(text);
    _m.sendAT(GF("+CMGF=1"));
    if (_m.waitResponse(2000L) != 1) {
      Serial.println(F("[SIM] The modem refused text mode (AT+CMGF=1)"));
      return false;
    }
    _m.sendAT(wide ? GF("+CSCS=\"UCS2\"") : GF("+CSCS=\"GSM\""));
    if (_m.waitResponse(2000L) != 1 && wide) {
      Serial.println(F("[SIM] The modem refused UCS2 (Arabic) — the message can't go"));
      return false;
    }
    _m.sendAT(wide ? GF("+CSMP=17,167,0,8") : GF("+CSMP=17,167,0,0"));
    _m.waitResponse(2000L);

    if (wide) _m.sendAT(GF("+CMGS=\""), ucs2Hex(String(to), 24), GF("\""));
    else _m.sendAT(GF("+CMGS=\""), to, GF("\""));
    bool ok = false;
    if (_m.waitResponse(10000L, GF(">")) == 1) {
      if (wide) _m.stream.print(ucs2Hex(text, 70));
      else _m.stream.print(text);
      _m.stream.write(static_cast<char>(0x1A));
      _m.stream.flush();
      // "+CMGS: <n>" then OK when the network took it; "+CMS ERROR: <code>" when it didn't
      String reply;
      const int8_t r = _m.waitResponse(60000L, reply, GF("+CMGS:"), GF("+CMS ERROR:"), GF("ERROR"));
      ok = r == 1;
      if (ok) _m.waitResponse(2000L);
      else {
        _m.waitResponse(1000L, reply);
        reply.trim();
        Serial.print(F("[SIM] The network refused the message: "));
        Serial.println(reply.length() ? reply : String(F("no answer in 60 s")));
      }
    } else {
      // no ">" prompt: the modem didn't take the number; leave the prompt if it came late
      _m.stream.write(static_cast<char>(0x1B));
      Serial.println(F("[SIM] The modem didn't take the number (no > prompt)"));
    }
    if (wide) {
      _m.sendAT(GF("+CSCS=\"GSM\""));
      _m.waitResponse(2000L);
      _m.sendAT(GF("+CSMP=17,167,0,0"));
      _m.waitResponse(2000L);
    }
    return ok;
  }

  /** the last 9 digits: 01012345678, +201012345678 and 00201012345678 are the same phone */
  static String tail(const String& s) {
    String d;
    for (size_t i = 0; i < s.length(); ++i)
      if (isDigit(s[i])) d += s[i];
    return d.length() > 9 ? d.substring(d.length() - 9) : d;
  }

  /** the board's own number this message claims to come from, or null */
  const char* allowed(const String& from) const {
    const String t = tail(from);
    if (t.length() < 6) return nullptr;
    for (uint8_t i = 0; i < 3; ++i)
      if (_allowed[i] && *_allowed[i] && tail(String(_allowed[i])) == t) return _allowed[i];
    return nullptr;
  }

  /** "STATUS", "status", "?", "؟", "حالة", "الحالة", "حاله", "حالة؟" */
  static bool asksStatus(const String& body) {
    String t = body;
    t.trim();
    if (t == "?" || t == "\xD8\x9F") return true;  // ? ؟
    t = loose(t);
    return t == "status" || t == "stat" || t == "s" || t == "\xD8\xAD\xD8\xA7\xD9\x84\xD9\x87";  // حاله
  }

  /**
   * A message as words to compare: English in small letters, Arabic with its letter forms made one (أ إ آ → ا,
   * ة → ه, ى → ي, no short vowels or tatweel) and "ال" off the front of each word, punctuation as spaces, one space
   * between words. "الحرارة كام؟" → "حراره كام".
   */
  static String loose(const String& in) {
    String s;
    s.reserve(in.length());
    const uint8_t* p = (const uint8_t*)in.c_str();
    while (*p) {
      const uint8_t c = p[0];
      if ((c == 0xD8 || c == 0xD9) && p[1]) {
        uint16_t u = ((c & 0x1F) << 6) | (p[1] & 0x3F);
        p += 2;
        if (u == 0x0623 || u == 0x0625 || u == 0x0622) u = 0x0627;           // أ إ آ → ا
        else if (u == 0x0629) u = 0x0647;                                     // ة → ه
        else if (u == 0x0649) u = 0x064A;                                     // ى → ي
        else if (u == 0x0640 || (u >= 0x064B && u <= 0x0652)) continue;       // tatweel, short vowels
        else if (u == 0x061F || u == 0x060C || u == 0x061B) u = ' ';          // ؟ ، ؛
        if (u == ' ') s += ' ';
        else {
          s += (char)(0xC0 | (u >> 6));
          s += (char)(0x80 | (u & 0x3F));
        }
      } else if (c < 0x80) {
        s += isAlphaNumeric(c) ? (char)tolower(c) : ' ';
        ++p;
      } else {
        s += (char)c;  // other letters, byte by byte as they are
        ++p;
      }
    }
    String out;
    const int n = s.length();
    int i = 0;
    while (i < n) {
      while (i < n && s[i] == ' ') ++i;
      int j = i;
      while (j < n && s[j] != ' ') ++j;
      if (j > i) {
        String w = s.substring(i, j);
        // "ال" and at least two letters after it
        if (w.length() >= 8 && w.startsWith("\xD8\xA7\xD9\x84")) w = w.substring(4);
        if (out.length()) out += ' ';
        out += w;
      }
      i = j;
    }
    return out;
  }

  /** The answer a message asks for: a word of yours (on its own or among others), else the status; or none. */
  Answer answerFor(const String& body) const {
    const String b = String(' ') + loose(body) + ' ';
    for (uint8_t i = 0; i < _nWords; ++i) {
      const String w = loose(String(_words[i]));
      if (w.length() && b.indexOf(String(' ') + w + ' ') >= 0) return _replies[i];
    }
    return _status && asksStatus(body) ? _status : nullptr;
  }

  /** Three answers in three minutes at most: a flood of questions can't spend the SIM's credit. */
  bool mayAnswer() {
    const uint32_t now = millis();
    uint32_t& oldest = _answers[_answerAt];
    if (oldest && now - oldest < 180000UL) return false;
    oldest = now | 1;
    _answerAt = (_answerAt + 1) % 3;
    return true;
  }

  /** "0030003100320033" (UCS-2 in hex, as the modem lists messages in UCS2 mode) → UTF-8 */
  static String fromUcs2(const String& s) {
    if (s.length() < 4 || s.length() % 4) return s;
    for (size_t i = 0; i < s.length(); ++i)
      if (!isHexadecimalDigit(s[i])) return s;
    String out;
    for (size_t i = 0; i + 3 < s.length(); i += 4) {
      const uint16_t u = (uint16_t)strtoul(s.substring(i, i + 4).c_str(), nullptr, 16);
      if (u < 0x80) out += (char)u;
      else if (u < 0x800) {
        out += (char)(0xC0 | (u >> 6));
        out += (char)(0x80 | (u & 0x3F));
      } else {
        out += (char)(0xE0 | (u >> 12));
        out += (char)(0x80 | ((u >> 6) & 0x3F));
        out += (char)(0x80 | (u & 0x3F));
      }
    }
    return out;
  }

  /** the n-th "quoted" field of a line */
  static String field(const String& line, uint8_t n) {
    int from = -1;
    for (uint8_t k = 0; k <= n; ++k) {
      from = line.indexOf('"', from + 1);
      if (from < 0) return String();
      const int to = line.indexOf('"', from + 1);
      if (to < 0) return String();
      if (k == n) return line.substring(from + 1, to);
      from = to;
    }
    return String();
  }

  /**
   * Reads the messages on the SIM. A question from one of the board's numbers gets its answer; every message
   * read is deleted (the SIM holds only a few dozen). One that couldn't be read whole stays for the next time.
   */
  void readInbox() {
    // text mode in the GSM alphabet: the sender's number comes as it is, and a message written in Arabic (UCS-2)
    // comes as its hex digits (3GPP 27.005), turned back into letters below
    _m.sendAT(GF("+CMGF=1"));
    if (_m.waitResponse() != 1) return;
    _m.sendAT(GF("+CSCS=\"GSM\""));
    _m.waitResponse();
    _m.sendAT(GF("+CMGL=\"ALL\""));
    String data;
    const int8_t r = _m.waitResponse(10000L, data);
    if (r != 1) return;

    int pos = 0;
    while ((pos = data.indexOf("+CMGL:", pos)) >= 0) {
      const int headEnd = data.indexOf('\n', pos);
      if (headEnd < 0) break;
      const String head = data.substring(pos, headEnd);
      int bodyEnd = data.indexOf('\n', headEnd + 1);
      if (bodyEnd < 0) bodyEnd = data.length();
      String body = data.substring(headEnd + 1, bodyEnd);
      body.trim();
      pos = bodyEnd;

      const int index = head.substring(6).toInt();
      const String from = field(head, 1);
      body = fromUcs2(body);
      _m.sendAT(GF("+CMGD="), index);
      _m.waitResponse();

      const char* owner = allowed(from);
      if (!owner) {
        Serial.print(F("[SIM] A message from a number not on this board — deleted: "));
        Serial.println(from);
        continue;
      }
      Serial.print(F("[SIM] Message from "));
      Serial.print(from);
      Serial.print(F(": "));
      Serial.println(body);
      const Answer fn = answerFor(body);
      if (!fn) continue;
      // a sender's number can be faked: the answer goes to the number stored on the board, never to the one
      // the message shows — a faked question reaches its real owner, not whoever faked it
      if (!mayAnswer()) {
        Serial.println(F("[SIM] Three answers in the last three minutes — this question is skipped"));
        continue;
      }
      sms(owner, fn());
    }
  }

  friend class ATG_PhoneAlert;
};

inline void ATG_PhoneAlert::turnOn(float value) {
  if (_on) return;
  _on = true;
  if (_went && millis() - _at < _gap) return;
  String t(_text);
  if (!isnan(value)) {
    t += " (";
    t += String(value, 1);
    t += ")";
  }
  // the messages first (they're quick), then the calls
  if (_sms)
    for (uint8_t i = 0; i < _n; ++i) _phone.sms(_to[i], t, this);
  if (_call)
    for (uint8_t i = 0; i < _n; ++i) _phone.call(_to[i], this);
  _went = true;
  _at = millis();
}
