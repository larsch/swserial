// SwSerial relay demo
//
// Relay serial communication between swSerial and standard Serial.

#define RX D5
#define TX D6
#define BAUD 9600

#ifdef USE_SWSERIAL
#include "swserial.h"
SwSerial swSerial(RX, TX);
#endif

#ifdef USE_SOFTWARESERIAL
#include "SoftwareSerial.h"
SoftwareSerial swSerial(RX, TX);
#endif

void setup()
{
  Serial.begin(115200);
  swSerial.begin(BAUD);
}

void loop()
{
  if (Serial.available())
    swSerial.write(Serial.read());
  if (swSerial.available())
    Serial.write(swSerial.read());
}
