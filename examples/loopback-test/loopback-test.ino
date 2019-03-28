// Software Serial loopback test

#define RX_PIN D6
#define TX_PIN D5

#include "fifo.h"
#include "lfsr.h"
#include "swserial.h"

#define TEST_CONFIGS 1
#include "swserial.h"
SwSerial swSerial(RX_PIN, TX_PIN);

// #define TEST_CONFIGS 0
// #include "SoftwareSerial.h"
// SoftwareSerial swSerial(RX_PIN, TX_PIN);

/** Number of bytes in error rate test */
#define TEST_BYTES 16384

/** Number of bytes to keep in buffer during test */
#define BUFFER_SIZE 4

/** Last byte sent */
static uint8_t currentByte = 0x00;

/** Rates to test */
static const int bauds[] = {300, 600, 1200, 2400, 4800, 9600, 19200};

/** Rates to test for error rate */
static const int errorbauds[] = {38400, 76800, 115200, 124800};

/** Baud rate index */
int baud_index = 0;

/** Number of bytes left to test */
int bytesLeft = 0;

/** Number of failed bytes */
int failures = 0;

/** PRNG byte sequence generator */
static lfsr16 nextByte;

SerialConfig configs[] = {
#if TEST_CONFIGS
    SERIAL_5N1, SERIAL_6N1, SERIAL_7N1,
#endif
    SERIAL_8N1,
#if TEST_CONFIGS
    SERIAL_5N2, SERIAL_6N2, SERIAL_7N2, SERIAL_8N2, SERIAL_5E1,
    SERIAL_6E1, SERIAL_7E1, SERIAL_8E1, SERIAL_5E2, SERIAL_6E2,
    SERIAL_7E2, SERIAL_8E2, SERIAL_5O1, SERIAL_6O1, SERIAL_7O1,
    SERIAL_8O1, SERIAL_5O2, SERIAL_6O2, SERIAL_7O2, SERIAL_8O2,
#endif
};

static const char *name(SerialConfig config)
{
  switch (config) {
  case SERIAL_5N1:
    return "5N1";
  case SERIAL_6N1:
    return "6N1";
  case SERIAL_7N1:
    return "7N1";
  case SERIAL_8N1:
    return "8N1";
  case SERIAL_5N2:
    return "5N2";
  case SERIAL_6N2:
    return "6N2";
  case SERIAL_7N2:
    return "7N2";
  case SERIAL_8N2:
    return "8N2";
  case SERIAL_5E1:
    return "5E1";
  case SERIAL_6E1:
    return "6E1";
  case SERIAL_7E1:
    return "7E1";
  case SERIAL_8E1:
    return "8E1";
  case SERIAL_5E2:
    return "5E2";
  case SERIAL_6E2:
    return "6E2";
  case SERIAL_7E2:
    return "7E2";
  case SERIAL_8E2:
    return "8E2";
  case SERIAL_5O1:
    return "5O1";
  case SERIAL_6O1:
    return "6O1";
  case SERIAL_7O1:
    return "7O1";
  case SERIAL_8O1:
    return "8O1";
  case SERIAL_5O2:
    return "5O2";
  case SERIAL_6O2:
    return "6O2";
  case SERIAL_7O2:
    return "7O2";
  case SERIAL_8O2:
    return "8O2";
  }
}

static int dataBits(SerialConfig config)
{
  switch (config) {
  case SERIAL_5E1:
  case SERIAL_5E2:
  case SERIAL_5N1:
  case SERIAL_5N2:
  case SERIAL_5O1:
  case SERIAL_5O2:
    return 5;
  case SERIAL_6E1:
  case SERIAL_6E2:
  case SERIAL_6N1:
  case SERIAL_6N2:
  case SERIAL_6O1:
  case SERIAL_6O2:
    return 6;
  case SERIAL_7E1:
  case SERIAL_7E2:
  case SERIAL_7N1:
  case SERIAL_7N2:
  case SERIAL_7O1:
  case SERIAL_7O2:
    return 7;
  case SERIAL_8E1:
  case SERIAL_8E2:
  case SERIAL_8N1:
  case SERIAL_8N2:
  case SERIAL_8O1:
  case SERIAL_8O2:
    return 8;
  }
}

static uint8_t dataMask(SerialConfig config)
{
  return (uint8_t)((1 << dataBits(config)) - 1);
}

static int serialConfigIndex = 0;

void beginTest()
{
  int baud = errorbauds[baud_index];
  baud_index = (baud_index + 1) % (sizeof(errorbauds) / sizeof(errorbauds[0]));
  Serial.printf("Testing loopback error rate from loop() at %u baud...", baud);
  swSerial.begin(baud);
  bytesLeft = TEST_BYTES;
  failures = 0;
  delayMicroseconds(1000);
  swSerial.write(currentByte = nextByte());
}

void testBuffered(int baud, SerialConfig config)
{
#if TEST_CONFIGS
  swSerial.begin(baud, config);
#else
  swSerial.begin(baud);
#endif
  Serial.printf("Testing buffered loopback at %u baud, %s...", baud,
                name(config));
  uint8_t mask = dataMask(config);
  bool pass = true;
  for (int i = 0; i < BUFFER_SIZE; ++i)
    swSerial.write(i);
  for (int i = 0; i < 256; ++i) {
    uint8_t received = swSerial.read();
    uint8_t expected = (i & mask);
    if (expected != received) {
      Serial.printf("Expected 0x%02x, got 0x%02x\r\n", expected, received);
      pass = false;
    }
    if (i < 256 - BUFFER_SIZE)
      swSerial.write(i + BUFFER_SIZE);
    ESP.wdtFeed();
  }
  Serial.println(pass ? "OK" : "FAIL");
}

void bitTest(int baud, SerialConfig config)
{
#if TEST_CONFIGS
  swSerial.begin(baud, config);
#else
  swSerial.begin(baud);
#endif
  lfsr16 in;

  Serial.printf("Testing loopback error rate at %u baud, %s...", baud,
                name(config));
  int good = 0;
  int bad = 0;
  const int count = 16384;
  const uint8_t mask = dataMask(config);
  unsigned int timeouts_left = 10;
  for (int i = 0; timeouts_left && i < count; ++i) {
    uint8_t expected = in();
    swSerial.write(expected);
    uint32_t one_bit_micros = (1000000 + baud - 1) / baud;
    uint32_t timeout = micros() + one_bit_micros;
    if (!swSerial.available())
      delayMicroseconds(one_bit_micros);
    if (!swSerial.available())
      ++bad;
    while (swSerial.available()) {
      uint8_t readByte = swSerial.read();
      if (readByte != uint8_t(expected & mask))
        ++bad;
    }
    ESP.wdtFeed();
  }
  if (!timeouts_left)
    Serial.print(" gave up, too many timeouts ");
  Serial.printf("%u/%u\r\n", bad, count);
}

#include <ESP8266WiFi.h>

void setup()
{
  Serial.begin(115200);

  // WiFi.begin("", "");
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(10);
  // }
  // Serial.println(WiFi.localIP());

  for (int i = 0; i < sizeof(configs) / sizeof(configs[0]); ++i) {
    SerialConfig config = configs[i];
    for (auto baud : bauds)
      testBuffered(baud, config);
    for (auto baud : errorbauds)
      bitTest(baud, config);
  }

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