// SwSerial relay demo
//
// Relay serial communication between swSerial and standard Serial.

#include <ESP8266CMD.h>
#include <ESP8266WiFi.h>
#include <swserial.h>

#define RX D5
#define TX D6
#define BAUD 9600

#include "swserial.h"
SwSerial swSerial(RX, TX);

ESP8266CMD cmd;

void send(Stream *str, int argc, const char **argv)
{
  if (argc == 2) {
    swSerial.write(argv[1], strlen(argv[1]));
  } else {
    Serial.println("Usage: send \"<text>\"");
  }
}

#define HDLC_FRAME 0x7E
#define HDLC_ESC 0x7D
#define HDLC_XOR 0x20

void hdlcSend(Stream &str, const uint8_t *data, size_t length)
{
  str.write(HDLC_FRAME);
  for (size_t i = 0; i < length; ++i) {
    uint8_t b = data[i];
    if (b == HDLC_FRAME || b == HDLC_ESC) {
      str.write(HDLC_ESC);
      b ^= HDLC_XOR;
    }
    str.write(b);
  }
  str.write(HDLC_FRAME);
}

static int hdlcLength = 0;
static uint8_t hdlcBuffer[64];
bool hdlcReceive(Stream &str)
{
  static int state = 0;
  while (str.available()) {
    uint8_t b = str.read();
    if (b == HDLC_FRAME) {
      if (state == 0) {
        len = 0;
        state = 1;
        return false;
      } else if (state == 1) {
        state = 0; // complete
        return true;
      } else {
        state = 0; // error
        return false;
      }
    } else if (state == 1) {
      if (b == HDLC_ESC) {
        state = 2;
      } else {
        buffer[len++] = b;
      }
    } else {
      buffer[len++] = b ^ HDLC_XOR;
    }

    if (state == 0) {
      if (b == HDLC_FRAME)
        state = 1;
    } else
  }
}

void baud(Stream *str, int argc, const char **argv)
{
  if (argc == 2) {
    int baud = atoi(argv[1]);
    if (baud >= 30 && baud <= 1000000) {
      swSerial.begin(baud);
      Serial.print("Baud rate set to ");
      Serial.println(baud);
    } else {
      Serial.println("Invalid baud rate");
    }
  } else {
    Serial.println("Usage: baud <baudrate>");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP8266CMD ready");
  swSerial.begin(9600);
  cmd.addCommand("baud", baud);
  cmd.addCommand("send", send);
  cmd.begin();
}

void loop() { cmd.run(); }
