

#define RX_PIN D5
#define TX_PIN D6

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
    return rx_fifo.size();
  }

  virtual int read()
  {
    rx();
    if (rx_fifo.empty())
      return -1;
    else
      return rx_fifo.get();
  }

  void dump(const char *s) { (void)s; }

  virtual int peek()
  {
    if (rx_fifo.empty())
      return -1;
    else
      return rx_fifo.peek();
  }

  void begin(int baud)
  {
    auto freq = ESP.getCpuFreqMHz() * 1000000;
    this->baud = baud;
    bit_length = freq / baud;
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

  void complete()
  {
    DEBUG(" complete 0x%02x", current);
    rx_fifo.add(current);
    current = 0;
    bitcount = 0;
  }

  void handle_edge_event(uint32_t time, unsigned int level)
  {
    DEBUG("time:%d level:%d ", time, level);
    if (bitcount == 0) {
      if (level == 0) {
        bitcount = 1; // stop bit found
      }
    } else {
      // time since last edge
      uint32_t delta = time - last_edge_time;

      // bits since last edge (rounded)
      uint32_t bits = (delta + bit_length / 2) / bit_length;

      // pad previous bit if interval was greater than 1 bit
      if (bits > 1) {
        int interrim_bits = bits - 1;
        if (bitcount + interrim_bits > 9)
          interrim_bits = 9 - bitcount;
        current =
            ((level - 1) << (8 - interrim_bits)) | (current >> interrim_bits);
        bitcount += interrim_bits;
        dump("interrim");
      }

      // add found bit
      if (bitcount < 9 && bits) {
        current = (level << 7) | (current >> 1);
        ++bitcount;
        dump("addbit  ");
      }

      // store if we have received all bits
      if (bitcount == 9) {
        rx_fifo.add(current);
        dump("storebyt");
        current = 0;
        bitcount = 0;
      }
    }
    last_edge_time = time;
    DEBUG("\r\n");
  }

  void check_time(uint32_t time)
  {
    if (bitcount > 0) {
      uint32_t passed = time - last_edge_time;
      if (bitcount * bit_length + passed > 10 * bit_length) {
        int missed = 9 - bitcount;
        uint8_t level = current >> 7;
        current = (((1 - level) - 1) << (8 - missed)) | (current >> missed);
        rx_fifo.add(current);
        dump("complbit");
        current = 0;
        bitcount = 0;
      }
    }
  }

  void rx()
  {
    if (input == output)
      check_time(ESP.getCycleCount());
    while (input != output) {
      uint32_t message = buffer[output];
      output = (output + 1) % BUFFER_SIZE;
      uint8_t level = message & 1;
      handle_edge_event(message, level);
    }
  }

  void rx2()
  {
    if (input == output && bitcount > 0) {
      DEBUG("bitcount %u", bitcount);
      uint32_t passed = ESP.getCycleCount() - last_edge_time;
      if (passed + bitcount * bit_length > 10 * bit_length + bit_length / 2) {
        // missing stop bit edge; insert final high bits and complete byte
        auto missed = 9 - bitcount;
        current = (0xFF << (bitcount - 1)) | (current >> missed);
        complete();
        DEBUG("\r\n");
      }
    }

    while (input != output) {
      uint32_t message = buffer[output];
      output = (output + 1) % BUFFER_SIZE;
      uint8_t level = message & 1;
      DEBUG("bitcount:%u message:%u level:%u", bitcount, message, level);
      if (bitcount == 0) {
        if (level == 0) {
          bitcount = 1;
          DEBUG(" start bitcount:%u", bitcount);
        }
      } else {
        uint32_t delta = message - last_edge_time;
        DEBUG(" delta:%u", delta);
        uint32_t bits = (delta + bit_length / 2) / bit_length;
        DEBUG(" bits:%u", bits);
        while (bitcount < 9 && bits > 1) {
          current = (current & 0x80) | (current >> 1);
          DEBUG(" shr current:0x%02x", current);
          --bits;
          ++bitcount;
        }

        DEBUG(" bitcount:%u", bitcount);
        if (bitcount < 9 && bits == 1) {
          current = (level << 7) | (current >> 1); /* add bit */
          --bits;
          ++bitcount;
          DEBUG(" addbit current:%u", current);
        }
        DEBUG(" bitcount:%u", bitcount);

        if (bitcount == 9)
          complete();

        // More bits than needed to complete byte; this is the start bit edge
        if (bits && level == 0) {
          // dumpBuffer();
          // Serial.printf("bitcount:%u bits:%u hadbits:%u level:%u "
          //               "current:0x%02x delta:%u START\r\n",
          //               bitcount, bits, (delta + bit_length / 2) /
          //               bit_length, level, current, delta);
          bitcount = 1;
        }

        DEBUG(" current:0x%02x", current);
      }
      last_edge_time = message;
      DEBUG("\r\n");
    }
  }

  void dumpBuffer();

  int txPin;
  uint32_t last_edge_time;
  uint32_t bit_length;
  TRACE(uint8_t, current) = 0;
  uint8_t bitcount = 0;
  FIFO<uint8_t, RX_BUFFER_SIZE> rx_fifo;
  int baud;
};

void SwSerial::dumpBuffer()
{
  for (int i = 0; i < BUFFER_SIZE; ++i) {
    Serial.print(buffer[i] >> 1);
    Serial.print(" ");
    Serial.print(buffer[i] & 1);
    int last = (i + BUFFER_SIZE - 1) % BUFFER_SIZE;
    auto delta = (buffer[i] - buffer[last]) >> 1;
    Serial.print(" ");
    Serial.print(delta);
    float bits = delta / (float)bit_length;
    Serial.print(" ");
    Serial.print(bits);
    if (i == output)
      Serial.print(" out");
    if (i == input)
      Serial.print(" in");
    Serial.println();
  }
}

SwSerial swSerial(RX_PIN, TX_PIN);

#define STARTBAUD 38400
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
  Serial.printf("wrote 0x%02x\r\n", b);
}

void setup()
{
  Serial.begin(115200);
  beginTest(baud);
}

void loop()
{
  if (swSerial.available()) {
    auto rd = swSerial.read();
    // Serial.print("read ");
    // Serial.println(rd, 16);
    if (rd == b) {
    } else {
      ++failures;
      Serial.printf("%u: Failed at 0x%02x, got 0x%02x %u (last was 0x%02x)\r\n",
                    baud, b, rd, bytesLeft, lastByte);
      delay(20);
    }
    bytesLeft--;
    if (bytesLeft) {
      if (bytesLeft % 2048 == 0)
        Serial.print('.');
      lastByte = b;
      swSerial.write(b = nextByte());
    } else {
      Serial.printf("%u/%u\r\n", failures, TEST_BYTES);
      if (baud < STARTBAUD)
        baud *= 2;
      else if (baud < 115200)
        baud += 38400;
      else
        baud = STARTBAUD;
      beginTest(baud);
    }
  }
}