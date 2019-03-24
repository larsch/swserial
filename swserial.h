// Copyright (C) Lars Christensen

#ifndef _swserial_h_
#define _swserial_h_

#include "bitreader.h"
#include "Arduino.h"

class SwSerial : Stream
{
public:
  BitReader br;
  SwSerial(int rx, int tx);
  void begin(int baud);

  virtual int available();
  virtual int read();
  virtual int peek();
  virtual size_t write(uint8_t b);

private:
  void rx();
  int txPin;
  unsigned int bit_length;
  uint32_t stopbit_edge_time = 0;
};

#endif // _swserial_h_