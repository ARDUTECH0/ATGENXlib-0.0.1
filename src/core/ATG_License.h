#pragma once
#include <Arduino.h>

namespace atg {

class License {
public:
  static void setKey(const String& key);
  static bool isValid();
  static bool isPro();
  static bool isIndustrial();

private:
  static bool verify(const String& key);
  static uint32_t simpleHash(const String& input);

  static String _key;
  static bool _valid;
  static bool _pro;
  static bool _industrial;
};

}