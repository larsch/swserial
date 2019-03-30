// SwSerial relay demo
//
// Relay serial communication between swSerial and standard Serial.

#define RX D5
#define TX D6
#define BAUD 300

#include "swserial.h"
SwSerial swSerial(RX, TX);

void setup()
{
  Serial.begin(115200);
  Serial.println("echo.ino");
  swSerial.begin(BAUD);
}

void loop()
{
  if (swSerial.available()) {
    uint8_t b = swSerial.read();
    swSerial.write(b);
    Serial.write(b);
  }
}
