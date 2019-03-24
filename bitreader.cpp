// Copyright (C) Lars Christensen

#include "bitreader.hpp"

void dump(const char*){}

void BitReader::edge(uint32_t time, int level)
{
  // check_time(time);
//   printf("time:%d level:%d ", time, level);
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
      bits -= interrim_bits;
      dump("interrim");
    }

    // add found bit
    if (bitcount < 9 && bits) {
      current = (level << 7) | (current >> 1);
      ++bitcount;
      --bits;
      dump("addbit  ");
    }

    // store if we have received all bits
    if (bitcount == 9) {
      rx_fifo.add(current);
      dump("storebyt");
      current = 0;
      bitcount = 0;
    }

    if (bits && level == 0) {
      bitcount = 1; // next start bit seen
    }
  }
  last_edge_time = time;
}

void BitReader::check(uint32_t time)
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
