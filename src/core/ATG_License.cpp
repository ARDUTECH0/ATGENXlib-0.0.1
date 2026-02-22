#include "ATG_License.h"
#include "../boards/ATG_Board.h"

namespace atg {

String License::_key = "";
bool License::_valid = false;
bool License::_pro = false;
bool License::_industrial = false;

#define ATG_SECRET_SALT "ATGENX_2026_SECRET"

void License::setKey(const String& key) {
  _key = key;
  _valid = verify(key);
}

bool License::isValid() { return _valid; }
bool License::isPro() { return _valid && _pro; }
bool License::isIndustrial() { return _valid && _industrial; }

bool License::verify(const String& key) {
  if (key.length() < 10) return false;

  // مثال بسيط parsing
  if (key.indexOf("PRO") > 0) _pro = true;
  if (key.indexOf("IND") > 0) _industrial = true;

  // استخراج آخر 8 حروف
  String hashPart = key.substring(key.length() - 8);

  String base = key.substring(0, key.length() - 9); // بدون dash
  String combined = base + ATG_SECRET_SALT;

  uint32_t h = simpleHash(combined);

  char buf[9];
  sprintf(buf, "%08X", h);

  return hashPart.equalsIgnoreCase(String(buf));
}

uint32_t License::simpleHash(const String& input) {
  uint32_t hash = 5381;
  for (size_t i = 0; i < input.length(); i++) {
    hash = ((hash << 5) + hash) + input[i];
  }
  return hash;
}

}