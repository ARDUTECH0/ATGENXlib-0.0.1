/*
 * Phone_Alerts — a LILYGO T-A7670X that texts and rings you when the room gets too hot, and answers
 * "STATUS" (or "حالة") with the temperature. No internet: the SIM needs credit for messages and calls only.
 *
 * Needs an ESP32 SIM board and LILYGO's TinyGSM (the one with TINY_GSM_MODEM_A7670):
 * https://atgenx.com/downloads/TinyGSM-LILYGO.zip — and the DHT sensor library.
 * Wiring: a DHT22 on GPIO 32. The modem's own pins (4, 5, 12, 25, 26, 27) are the board's.
 */
#if defined(ESP32) && __has_include(<TinyGsmClientA7670.h>)

#define TINY_GSM_MODEM_A7670
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>
#include <ATG_Phone.h>
#include <ATG_Logic.h>

TinyGsm modem(Serial1);
ATG_Phone phone(modem);

// one alert: a message and a call to two numbers, not again before 10 minutes
ATG_PhoneAlert hot(phone, "+201012345678", "+201198765432", "", "The room is too hot", 10UL * 60000UL, true, true);
ATG_DhtReader room(32, DHT22);

String status() {
  String s = F("Board status");
  s += "\nRoom: ";
  s += atgxNum(room.temperatureC(), 1, "C");
  s += " / ";
  s += atgxNum(room.humidity(), 0, "%");
  return s;
}

void setup() {
  Serial.begin(115200);
  // the T-A7670X's power-up sequence (LILYGO's)
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  delay(100);
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(4, LOW);
  Serial1.begin(115200, SERIAL_8N1, 27, 26);
  delay(3000);
  for (uint8_t i = 0; i < 10 && !modem.testAT(1000); ++i) delay(500);
  modem.init();
  modem.waitForNetwork(60000UL);

  phone.begin();
  phone.answer("+201012345678", "+201198765432", "", status);
  room.begin();
}

void loop() {
  phone.loop();
  room.poll();
  const float t = room.temperatureC();
  if (!isnan(t) && t > 40) hot.turnOn(t);
  else hot.turnOff();
}

#else

// another board, or TinyGSM without the A7670: say what's needed
void setup() {
  Serial.begin(115200);
  Serial.println(F("Phone_Alerts needs an ESP32 SIM board (LILYGO T-A7670X) and LILYGO's TinyGSM:"));
  Serial.println(F("https://atgenx.com/downloads/TinyGSM-LILYGO.zip"));
}

void loop() {}

#endif
