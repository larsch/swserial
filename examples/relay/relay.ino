36300d0bccG// SwSerial relay demo
//
// Relay serial communication between swSerial and standard Serial.

#define RX D5
#define TX D6
#define BAUD 9600

#include "swserial.h"
SwSerial swSerial(RX, TX);

void setup()
{
  Serial.begin(115200);
  Serial.println("relay.ino");
  swSerial.begin(BAUD);
}

void loop()
{
  if (Serial.available())
    swSerial.write(Serial.read());
  if (swSerial.available())
    Serial.write(swSerial.read());
}
