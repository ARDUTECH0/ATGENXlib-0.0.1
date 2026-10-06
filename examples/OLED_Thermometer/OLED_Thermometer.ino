// Temperature & humidity on a 128x64 SSD1306 OLED (I2C: SDA/SCL, address 0x3C).
// DHT22 data → pin 4.
#include <ATGENXlib.h>
#include <components/displays/ATG_OledDisplay.h>
using namespace atg;

Runtime rt;
DHTSensor room(4, DhtType::DHT22);
OledDisplay screen;

void draw(OledDisplay& s) {
  s.print(0, 0, "Room");
  s.print(0, 2, room.temperatureC(), 1, "\xB0" "C", 2);
  s.print(0, 5, "Humidity");
  s.print(10, 5, room.humidity(), 0, "%");
}

void setup() {
  rt.addModule(room);
  rt.addModule(screen);
  screen.onRender(draw);
  rt.begin();
}

void loop() {
  rt.loopOnce();
}
