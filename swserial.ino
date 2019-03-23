

#define RX_PIN D5
#define TX_PIN D6

#define BUFFER_SIZE 64
uint32_t buffer[BUFFER_SIZE];
uint32_t input = 0;
uint32_t output = 0;
int rxPin;

#define RX_BUFFER_SIZE 32

#define DEBUG(...)

static void isr()
{
  auto pinState = digitalRead(rxPin);
  uint32_t cycleCount = ESP.getCycleCount();
  uint32_t value = (cycleCount << 1) | pinState;
  buffer[input] = value;
  input = (input + 1) % BUFFER_SIZE;
}

static void delayUntil(uint32_t cycles)
{
  while (int32_t(cycles - ESP.getCycleCount()) > 0)
    ;
}

class SwSerial
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
    return (rxBufferIn + RX_BUFFER_SIZE - rxBufferOut) % RX_BUFFER_SIZE;
  }

  int read()
  {
    rx();
    if (rxBufferIn == rxBufferOut)
      return -1;
    else {
      uint8_t b = rxBuffer[rxBufferOut];
      rxBufferOut = (rxBufferOut + 1) % RX_BUFFER_SIZE;
      return b;
    }
  }

  void begin(int baud)
  {
    auto freq = ESP.getCpuFreqMHz() * 1000000;
    this->baud = baud;
    bitLength = freq / baud;
    // bitLengthFraction = freq % baud;
  }

  void write(uint8_t b)
  {
    uint32_t clock = ESP.getCycleCount();
    digitalWrite(txPin, LOW);
    for (int i = 0; i < 8; ++i) {
      delayUntil(clock += bitLength);
      digitalWrite(txPin, b & 1);
      b >>= 1;
    }
    delayUntil(clock += bitLength);
    digitalWrite(txPin, HIGH);
    delayUntil(clock += bitLength);
  }

  void complete()
  {
    DEBUG(" complete 0x%02x", currentByte);
    rxBuffer[rxBufferIn] = currentByte;
    rxBufferIn = (rxBufferIn + 1) % RX_BUFFER_SIZE;
    currentByte = 0;
    bitcount = 0;
  }

  void rx()
  {
    if (bitcount > 0) {
      DEBUG("bitcount %u", bitcount);
      uint32_t passed = ESP.getCycleCount() - last;
      if (passed > 10 * bitLength) {
        // missing stop bit edge; insert final high bits and complete byte
        auto missed = 9 - bitcount;
        currentByte = (0xFF << (bitcount - 1)) | (currentByte >> missed);
        complete();
        DEBUG("\r\n");
        return;
      }
    }

    while (input != output) {
      uint32_t message = buffer[output];
      output = (output + 1) % BUFFER_SIZE;
      uint32_t cycle = message >> 1;
      uint8_t level = message & 1;
      DEBUG("bitcount:%u cycle:%u level:%u", bitcount, cycle, level);
      if (bitcount == 0) {
        ++bitcount;
        DEBUG(" start bitcount:%u", bitcount);
      } else {
        uint32_t delta = cycle - last;
        DEBUG(" delta:%u", delta);
        uint32_t bits = (delta + bitLength / 2) / bitLength;
        DEBUG(" bits:%u", bits);
        bitcount += bits;
        while (bits > 1) {
          currentByte = (currentByte & 0x80) | (currentByte >> 1);
          DEBUG(" currentByteSh:0x%02x", currentByte);
          --bits;
        }
        DEBUG(" bitcount:%u", bitcount);
        if (bitcount < 10)
          currentByte = (level << 7) | (currentByte >> 1); /* add bit */
        else
          complete();
        DEBUG(" currentByte:0x%02x", currentByte);
      }
      last = cycle;
      DEBUG("\r\n");
    }
  }

  int txPin;
  uint32_t last;
  uint32_t bitLength;
  uint8_t currentByte = 0;
  uint8_t bitcount = 0;
  uint8_t rxBuffer[RX_BUFFER_SIZE];
  uint8_t rxBufferIn = 0;
  uint8_t rxBufferOut = 0;
  int baud;
};

SwSerial swSerial(RX_PIN, TX_PIN);

uint8_t b = 0x00;
int baud = 2400;

void beginTest(int baud)
{
  Serial.print(baud);
  swSerial.begin(baud);
  swSerial.write(b = 0x00);
}

void setup()
{
  Serial.begin(115200);
  baud = 2400;
  beginTest(baud);
}

void loop()
{
  if (swSerial.available()) {
    auto rd = swSerial.read();
    // Serial.println(rd, 16);
    if (rd == b) {
    } else {
      Serial.printf("Failed at 0x%02x, got 0x%02x\r\n", b, rd);
    }
    if (b != 0xFF) {
      ++b;
      swSerial.write(b);
    } else {
      Serial.printf("SUCCESS\r\n");
      baud *= 2;
      beginTest(baud);
    }
  }
}