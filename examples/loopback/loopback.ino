// SwSerial loopback demo
//
// Tests sending a byte and expects to receive same byte. TX pin must be
// connected directly to RX pin.

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
  swSerial.begin(BAUD, SERIAL_5E1);
}

void loop()
{
  static uint8_t currentByte = 0x00;

  swSerial.write(currentByte);
  Serial.print("Wrote ");
  Serial.print(currentByte, 16);

  uint32_t timeout = millis() + 1000;
  while (!swSerial.available()) {
    if (millis() > timeout) {
      Serial.println("Timeout");
      return;
    }
  }

  while (swSerial.available()) {
    uint8_t readByte = swSerial.read();
    Serial.print(", got back ");
    Serial.print(readByte, 16);
  }

  Serial.println();

  delay(500);

  currentByte += 1;
}
