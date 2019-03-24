// Copyright (C) Lars Christensen

#ifndef _bitreader_hpp_
#define _bitreader_hpp_

#include "fifo.h"
#include <cstdint>

class BitReader
{
public:
  void edge(uint32_t time, int level);
  void check(uint32_t time);
  FIFO<uint8_t, 32> rx_fifo;

private:
  unsigned int bitcount = 0;
  unsigned int bit_length = 100;
  uint32_t last_edge_time = 0;
  uint8_t current = 0;
};

#endif // _bitreader_hpp_