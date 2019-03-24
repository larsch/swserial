// Copyright (C) Lars Christensen

#include "swserial.hpp"

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

static void isr()
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

void SwSerial::begin(int baud)
{
  auto freq = CYCLE_FREQ;
  bit_length = freq / baud;
  br = BitReader(bit_length);
  // bit_lengthFraction = freq % baud;
}

int SwSerial::available()
{
  rx();
  return br.rx_fifo.size();
}

int SwSerial::read()
{
  rx();
  if (br.rx_fifo.empty())
    return -1;
  else
    return br.rx_fifo.get();
}

int SwSerial::peek()
{
  if (br.rx_fifo.empty())
    return -1;
  else
    return br.rx_fifo.peek();
}

size_t SwSerial::write(uint8_t b)
{
  rx();
  uint32_t clock = CYCLE_COUNT;
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

void SwSerial::rx()
{
  if (edge_fifo.empty())
    br.check(CYCLE_COUNT);
  while (!edge_fifo.empty()) {
    uint32_t message = edge_fifo.get();
    uint8_t level = message & 1;
    br.edge(message, level);
  }
}
