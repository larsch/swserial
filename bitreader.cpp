// Copyright (C) Lars Christensen

#include "bitreader.h"

void BitReader::edge(uint32_t time, int level)
{
  if (bitcount == 0) {
    if (level == 0) {
      bitcount = 1; // stop bit found
    }
  } else {
    // time since last edge
    uint32_t delta = time - last_edge_time;

    // bias if edge is late (brings down error rate from 2% to 0.5% at 115200 baud)
    uint32_t lateness = delta % bit_length;
    if (lateness < bit_length / 2) {
        time -= lateness / 2;
        delta -= lateness / 2;
    }

    // bits since last edge (rounded)
    uint32_t bits = (delta + bit_length / 2) / bit_length;

    // repeat previous bit if interval was greater than 1 bit
    if (bits > 1) {
      int interrim_bits = bits - 1;
      if (bitcount + interrim_bits > 9)
        interrim_bits = 9 - bitcount;
      current =
          ((level - 1) << (8 - interrim_bits)) | (current >> interrim_bits);
      bitcount += interrim_bits;
      bits -= interrim_bits;
    }

    // add found bit
    if (bitcount < 9 && bits) {
      current = (level << 7) | (current >> 1);
      ++bitcount;
      --bits;
    }

    // store if we have received all bits
    if (bitcount == 9) {
      rx_fifo.add(current);
      current = 0;
      bitcount = 0;
    }

    // check if this is the next start bit edge
    if (bits && level == 0) {
      bitcount = 1;
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
      current = 0;
      bitcount = 0;
    }
  }
}
