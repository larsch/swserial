// Copyright (C) Lars Christensen

#include "swserial.h"

#if defined(ARDUINO_ARCH_ESP8266)
#define CYCLE_FREQ (ESP.getCpuFreqMHz() * 1000000)
#define CYCLE_UFREQ ESP.getCpuFreqMHz()
#define CYCLE_COUNT ESP.getCycleCount()
#endif

static FIFO<uint32_t, 64> edge_fifo;
static int rxPin;

static inline int32_t timeUntil(uint32_t cycles)
{
  return int32_t(cycles - CYCLE_COUNT);
}

static inline void delayUntil(uint32_t cycles)
{
  int32_t microseconds = timeUntil(cycles) / CYCLE_UFREQ;
  if (microseconds > 1)
    delayMicroseconds(microseconds);
  while (timeUntil(cycles) > 0)
    ;
}

static void ICACHE_RAM_ATTR isr()
{
  uint32_t cycleCount = CYCLE_COUNT;
  auto pinState = !digitalRead(rxPin);
  edge_fifo.add((cycleCount | 1) ^ pinState);
}

SwSerial::SwSerial(int rx, int tx)
{
  rxPin = rx;
  txPin = tx;
  pinMode(rx, INPUT_PULLUP);
  digitalWrite(tx, HIGH);
  pinMode(tx, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(rxPin), isr, CHANGE);
}

void SwSerial::begin(int baud, SerialConfig config)
{
  auto freq = CYCLE_FREQ;
  bit_length = freq / baud;

  unsigned int databits;
  char parity;

  switch (config) {
  case SERIAL_5N1:
    databits = 5;
    parity = 0;
    stopbits = 1;
    break;
  case SERIAL_6N1:
    databits = 6;
    parity = 0;
    stopbits = 1;
    break;
  case SERIAL_7N1:
    databits = 7;
    parity = 0;
    stopbits = 1;
    break;
  case SERIAL_8N1:
    databits = 8;
    parity = 0;
    stopbits = 1;
    break;
  case SERIAL_5N2:
    databits = 5;
    parity = 0;
    stopbits = 2;
    break;
  case SERIAL_6N2:
    databits = 6;
    parity = 0;
    stopbits = 2;
    break;
  case SERIAL_7N2:
    databits = 7;
    parity = 0;
    stopbits = 2;
    break;
  case SERIAL_8N2:
    databits = 8;
    parity = 0;
    stopbits = 2;
    break;
  case SERIAL_5E1:
    databits = 5;
    parity = 'E';
    stopbits = 1;
    break;
  case SERIAL_6E1:
    databits = 6;
    parity = 'E';
    stopbits = 1;
    break;
  case SERIAL_7E1:
    databits = 7;
    parity = 'E';
    stopbits = 1;
    break;
  case SERIAL_8E1:
    databits = 8;
    parity = 'E';
    stopbits = 1;
    break;
  case SERIAL_5E2:
    databits = 5;
    parity = 'E';
    stopbits = 2;
    break;
  case SERIAL_6E2:
    databits = 6;
    parity = 'E';
    stopbits = 2;
    break;
  case SERIAL_7E2:
    databits = 7;
    parity = 'E';
    stopbits = 2;
    break;
  case SERIAL_8E2:
    databits = 8;
    parity = 'E';
    stopbits = 2;
    break;
  case SERIAL_5O1:
    databits = 5;
    parity = 'O';
    stopbits = 2;
    break;
  case SERIAL_6O1:
    databits = 6;
    parity = 'O';
    stopbits = 1;
    break;
  case SERIAL_7O1:
    databits = 7;
    parity = 'O';
    stopbits = 1;
    break;
  case SERIAL_8O1:
    databits = 8;
    parity = 'O';
    stopbits = 1;
    break;
  case SERIAL_5O2:
    databits = 5;
    parity = 'O';
    stopbits = 2;
    break;
  case SERIAL_6O2:
    databits = 6;
    parity = 'O';
    stopbits = 2;
    break;
  case SERIAL_7O2:
    databits = 7;
    parity = 'O';
    stopbits = 2;
    break;
  case SERIAL_8O2:
    databits = 8;
    parity = 'O';
    stopbits = 2;
    break;
  }

  Serial.print("bitlength ");
  Serial.print(bit_length);
  Serial.print(" databits ");
  Serial.print(databits);
  Serial.print(" parity ");
  Serial.print(parity ? parity : 'N');
  Serial.print(" stopbits ");
  Serial.println(stopbits);

  BitReader::begin(databits, parity);
}

int SwSerial::available()
{
  rx();
  return rx_fifo.size();
}

int SwSerial::read()
{
  rx();
  if (rx_fifo.empty())
    return -1;
  else
    return rx_fifo.get();
}

int SwSerial::peek()
{
  if (rx_fifo.empty())
    return -1;
  else
    return rx_fifo.peek();
}

uint8_t calc_parity(uint8_t b)
{
  return (0x6996 >> ((b ^ (b >> 4)) & 0xF)) & 1;
}

size_t SwSerial::write(uint8_t b)
{
  rx();

  uint_fast16_t bits = b & ((1 << databits) - 1);
  if (parity) {
    uint_fast16_t parity_bit = calc_parity(bits) ^ parity_odd;
    bits |= parity_bit << databits;
  }

  bits <<= 1; // start bit

  uint_fast8_t count = databits + !!parity + 1;
  // Serial.printf("0x%04x 0x%02x %d %u\n", bits, b, count, bit_length);

  uint32_t clock = CYCLE_COUNT;
  if (stopbit_edge_time - clock < bit_length) {
    // wait until previous stop bit is complete
    delayUntil(stopbit_edge_time);
  }

  clock = CYCLE_COUNT;

  for (uint_fast8_t i = 0; i < count; ++i) {
    digitalWrite(txPin, bits & 1);
    delayUntil(clock += bit_length);
    bits >>= 1;
  }

  // stop bit
  digitalWrite(txPin, HIGH);
  stopbit_edge_time = clock + bit_length * stopbits;
  return 1;
}

void SwSerial::rx()
{
  if (edge_fifo.empty())
    check(CYCLE_COUNT);
  while (!edge_fifo.empty()) {
    uint32_t message = edge_fifo.get();
    uint8_t level = message & 1;
    edge(message, level);
  }
}
