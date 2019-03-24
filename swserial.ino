

#define RX_PIN D5
#define TX_PIN D6

#include "bitreader.hpp"
#include "fifo.h"

#define BUFFER_SIZE 64
uint32_t buffer[BUFFER_SIZE];
uint32_t input = 0;
uint32_t output = 0;
int rxPin;

#define RX_BUFFER_SIZE 32

#define DEBUG_ENABLE 0

#if DEBUG_ENABLE
#define DEBUG(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

static void isr()
{
  uint32_t cycleCount = ESP.getCycleCount();
  auto pinState = !digitalRead(rxPin);
  buffer[input] = (cycleCount | 1) ^ pinState;
  input = (input + 1) % BUFFER_SIZE;
  if (input == output) {
    Serial.println("OVERFLOW");
    while (1)
      ;
  }
}

int32_t timeUntil(uint32_t cycles)
{
  return int32_t(cycles - ESP.getCycleCount());
}

static void delayUntil(uint32_t cycles)
{
  int32_t microseconds = timeUntil(cycles) / ESP.getCpuFreqMHz();
  if (microseconds > 1)
    delayMicroseconds(microseconds);
  while (timeUntil(cycles) > 0)
    ;
}

template <typename T, typename NM> class trace
{
public:
  trace(const T &v) : iv(v) {}
  trace<T, NM> &operator=(const T &v)
  {
    Serial.printf(" %s:%u", NM::nm(), v);
    iv = v;
    return *this;
  }
  bool operator==(const T &v) { return iv == v; }
  operator T() const { return iv; }
  T iv;
};

#if DEBUG_ENABLE
#define TRACE(type, name)                     \
  struct tr_##name {                          \
    static const char *nm() { return #name; } \
  };                                          \
  trace<type, tr_##name> name
#else
#define TRACE(type, name) type name
#endif

class SwSerial : Stream
{
public:
  BitReader br;
  SwSerial(int rx, int tx)
  {
    rxPin = rx;
    txPin = tx;
    pinMode(rx, INPUT_PULLUP);
    digitalWrite(tx, HIGH);
    pinMode(tx, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(rxPin), isr, CHANGE);
  }

  int available()
  {
    rx();
    return br.rx_fifo.size();
  }

  virtual int read()
  {
    rx();
    if (br.rx_fifo.empty())
      return -1;
    else
      return br.rx_fifo.get();
  }

  void dump(const char *s) { (void)s; }

  virtual int peek()
  {
    if (br.rx_fifo.empty())
      return -1;
    else
      return br.rx_fifo.peek();
  }

  void begin(int baud)
  {
    auto freq = ESP.getCpuFreqMHz() * 1000000;
    this->baud = baud;
    bit_length = freq / baud;
    br = BitReader(bit_length);
    // bit_lengthFraction = freq % baud;
  }

  virtual size_t write(uint8_t b)
  {
    rx();
    uint32_t clock = ESP.getCycleCount();
    digitalWrite(txPin, LOW);
    for (int i = 0; i < 8; ++i) {
      delayUntil(clock += bit_length);
      digitalWrite(txPin, b & 1);
      b >>= 1;
    }
    delayUntil(clock += bit_length);
    digitalWrite(txPin, HIGH);
    delayUntil(clock += bit_length);
    return 1;
  }

  void rx()
  {
    if (input == output)
      br.check(ESP.getCycleCount());
    while (input != output) {
      uint32_t message = buffer[output];
      output = (output + 1) % BUFFER_SIZE;
      uint8_t level = message & 1;
      br.edge(message, level);
    }
  }

  int txPin;
  uint32_t last_edge_time;
  uint32_t bit_length;
  TRACE(uint8_t, current) = 0;
  uint8_t bitcount = 0;
  int baud;
};

SwSerial swSerial(RX_PIN, TX_PIN);

#define STARTBAUD 9600
uint8_t b = 0x00;
int baud = STARTBAUD;
int bytesLeft = 0;
#define TEST_BYTES 16384
int failures = 0;

uint16_t lfsr = 0xace1u;
uint8_t nextByte()
{
  auto bit = lfsr & 1;
  lfsr >>= 1;
  if (bit)
    lfsr ^= 0xb400;
  return lfsr;
}

uint8_t lastByte = 0x00;

void beginTest(int baud)
{
  Serial.print(baud);
  swSerial.begin(baud);
  bytesLeft = TEST_BYTES;
  failures = 0;
  delayMicroseconds(1000);
  swSerial.write(b = nextByte());
}

void setup()
{
  Serial.begin(115200);
  beginTest(baud);
}

void loop()
{
  if (swSerial.available()) {
    while (swSerial.available()) {
      auto rd = swSerial.read();
      if (rd == b) {
      } else {
        ++failures;
        // Serial.printf("%u: Failed at 0x%02x, got 0x%02x %u (last was 0x%02x)\r\n",
        //               baud, b, rd, bytesLeft, lastByte);
        // delay(20);
      }
    }
    bytesLeft--;
    if (bytesLeft) {
      if (bytesLeft % 2048 == 0)
        Serial.print('.');
      lastByte = b;
      swSerial.write(b = nextByte());
    } else {
      Serial.printf("%u/%u\r\n", failures, TEST_BYTES);
      if (baud < 76800)
        baud *= 2;
      else if (baud < 115200)
        baud += 38400;
      else
        baud = STARTBAUD;
      beginTest(baud);
    }
  }
}