#define RX_PIN D5
#define TX_PIN D6

#include "swserial.h"
SwSerial swSerial(RX_PIN, TX_PIN);

void setup() { swSerial.begin(9600); }

void loop() { swSerial.write(0x55); }
