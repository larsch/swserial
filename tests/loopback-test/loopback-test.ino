// Software Serial loopback test

#define RX_PIN D5
#define TX_PIN D6

#include "fifo.h"
#include "lfsr.h"

#ifdef USE_SWSERIAL
#include "swserial.h"
SwSerial swSerial(RX_PIN, TX_PIN);
#endif

#ifdef USE_SOFTWARESERIAL
#include "SoftwareSerial.h"
SoftwareSerial swSerial(RX_PIN, TX_PIN);
#endif

/** Number of bytes in error rate test */
#define TEST_BYTES 16384

/** Number of bytes to keep in buffer during test */
#define BUFFER_SIZE 4

/** Last byte sent */
static uint8_t currentByte = 0x00;

/** Rates to test */
static const int bauds[] = {300, 600, 1200, 2400, 4800, 9600, 19200};

/** Rates to test for error rate */
static const int errorbauds[] = {38400, 76800, 115200, 153600};

/** Baud rate index */
int baud_index = 0;

/** Number of bytes left to test */
int bytesLeft = 0;

/** Number of failed bytes */
int failures = 0;

/** PRNG byte sequence generator */
static lfsr16 nextByte;

void beginTest()
{
  int baud = errorbauds[baud_index];
  baud_index = (baud_index + 1) % (sizeof(errorbauds)/sizeof(errorbauds[0]));
  Serial.printf("Testing loopback error rate from loop() at %u baud...", baud);
  swSerial.begin(baud);
  bytesLeft = TEST_BYTES;
  failures = 0;
  delayMicroseconds(1000);
  swSerial.write(currentByte = nextByte());
}

void testBuffered(int baud)
{
  swSerial.begin(baud);
  Serial.printf("Testing buffered loopback at %u baud...", baud);
  bool pass = true;
  for (int i = 0; i < BUFFER_SIZE; ++i)
    swSerial.write(i);
  for (int i = 0; i < 256; ++i) {
    uint8_t received = swSerial.read();
    if (i != received) {
      Serial.printf("Expected 0x%02x, got 0x%02x\r\n", i, received);
      pass = false;
    }
    if (i < 256 - BUFFER_SIZE)
      swSerial.write(i + BUFFER_SIZE);
    ESP.wdtFeed();
  }
  Serial.println(pass ? "OK" : "FAIL");
}

void bitTest(int baud)
{
  swSerial.begin(baud);
  lfsr16 in;

  Serial.printf("Testing loopback error rate at %u baud...", baud);
  int good = 0;
  int bad = 0;
  const int count = 16384;
  for (int i = 0; i < count; ++i) {
    uint8_t expected = in();
    swSerial.write(expected);
    while (!swSerial.available())
      /* wait for data */;
    while (swSerial.available()) {
      uint8_t readByte = swSerial.read();
      if (readByte != uint8_t(expected))
        ++bad;
    }
    ESP.wdtFeed();
  }
  Serial.printf("%u/%u\r\n", bad, count);
}

void setup()
{
  Serial.begin(115200);

  for (auto baud : bauds)
    testBuffered(baud);
  for (auto baud : errorbauds)
    bitTest(baud);

  beginTest();
}

void loop()
{
  if (swSerial.available()) {
    while (swSerial.available()) {
      auto readByte = swSerial.read();
      if (readByte != currentByte)
        ++failures;
    }
    bytesLeft--;
    if (bytesLeft) {
      swSerial.write(currentByte = nextByte());
    } else {
      Serial.printf("%u/%u\r\n", failures, TEST_BYTES);
      beginTest();
    }
  }
}