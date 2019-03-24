// SwSerial loopback demo
//
// Tests sending a byte and expects to receive same byte. TX pin must be
// connected directly to RX pin.

#include "swserial.hpp"

#define RX D5
#define TX D6
#define BAUD 9600

SwSerial swSerial(RX, TX);

void setup()
{
  Serial.begin(115200);
  swSerial.begin(BAUD);
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

  static uint8_t readByte = swSerial.read();
  Serial.print(", got back ");
  Serial.println(readByte, 16);

  delay(500);

  currentByte += 1;
}
