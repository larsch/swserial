// Copyright (C) Lars Christensen

#ifndef _swserial_h_
#define _swserial_h_

#include "bitreader.h"
#include "Arduino.h"

class SwSerial : public Stream, private BitReader
{
public:
  SwSerial(int rx, int tx);
  void begin(int baud, SerialConfig config = SERIAL_8N1);

  virtual int available();
  virtual int read();
  virtual int peek();
  virtual size_t write(uint8_t b);
  using Stream::write;

private:
  void rx();
  int txPin;
  uint32_t stopbit_edge_time = 0;
  int stopbits;
};

#endif // _swserial_h_