# SwSerial Software Serial port implementation for ESP8266/Arduino

This is an implementation of a software serial port API for the ESP8266/Arduino.
Works reliably up 76800 with neglible errors and up to 115200 with less than 1%
errors when WiFi is disabled.

## Example

```c++
#include "swserial.h"

#define RX D5
#define TX D6

SwSerial swSerial(RX, TX);

void setup()
{
    swSerial.begin(9600);
}

void loopback()
{
    if (swSerial.available())
        auto value = swSerial.read();

    swSerial.write(someByte);
}
```

## Why a new implementation?

The bundled SoftwareSerial library did not work reliably for me even at very low
baud rates (300). This library is tested at all baud rates from 300 to 115200.
The implementation is based on the same technique (measuring time between Rx pin
rising/falling edges), but is a (subjectively) simpler implementation. The
cycle/edge algorithm is fully unittested on PC including bit corruption (and
restoration of clock).

## Copyright

Copyright 2019 Lars Christensen. MIT License. See LICENSE.md.