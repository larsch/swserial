// Copyright (C) Lars Christensen

#ifndef _swserial_hpp_
#define _swserial_hpp_

#include "bitreader.hpp"
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
};

#endif // _swserial_hpp_