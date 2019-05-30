// Copyright (C) Lars Christensen

#include "bitreader.h"
#include <cstdio>

/** Size of 'current' register in bits */
#define CSIZE (sizeof(current) * 8)

#define printf(...)

void BitReader::edge(uint32_t time, int level)
{
  static unsigned int c = 0;
  ++c;
  printf("%u time:%u level:%u", c, time, level);
  if (bitcount == 0) {
    if (level == 0) {
      printf(" start%u", c);
      bitcount = 1; // start bit found
    }
  } else {
    // time since last edge
    uint32_t delta = time - last_edge_time;

    // bias if edge is late (brings down error rate from 2% to 0.5% at 115200
    // baud)
    uint32_t lateness = delta % bit_length;
    if (lateness < bit_length / 2) {
      time -= lateness / 2;
      delta -= lateness / 2;
    }

    // bits since last edge (rounded)
    uint32_t bits = (delta + bit_length / 2) / bit_length;

    if (bits == 0 && bitcount == 1 && level == 1) {
      bitcount = 0;
      return;
    }

    // repeat previous bit if interval was greater than 1 bit
    if (bits > 1) {
      int interrim_bits = bits - 1;
      if (bitcount + interrim_bits > expected_bits)
        interrim_bits = expected_bits - bitcount;
      current =
          ((level - 1) << (CSIZE - interrim_bits)) | (current >> interrim_bits);
      printf(" interrim:%u current:%x", interrim_bits, current);
      bitcount += interrim_bits;
      bits -= interrim_bits;
    }

    // add found bit
    if (bitcount < expected_bits && bits) {
      current = (level << (CSIZE - 1)) | (current >> 1);
      printf(" add current:%x", current);
      ++bitcount;
      --bits;
    }

    // store if we have received all bits
    if (bitcount == expected_bits) {
      printf(" output:%u", current);
      output(current);
      current = 0;
      bitcount = 0;
    }

    // check if this is the next start bit edge
    if (bits && level == 0) {
      bitcount = 1;
    }
  }
  last_edge_time = time;
  printf(" end:%u\r\n", c);
}

#include <stdio.h>

void BitReader::check(uint32_t time)
{
  if (bitcount > 0) {
    uint32_t passed = time - last_edge_time;
    int allbits = expected_bits + 1;
    if (bitcount * bit_length + passed > allbits * bit_length) {
      int missed = expected_bits - bitcount;
      uint8_t level = current >> (CSIZE - 1);
      uint32_t result =
          (((1 - level) - 1) << (CSIZE - missed)) | (current >> missed);
      output(result);
      current = 0;
      bitcount = 0;
    }
  }
}

uint8_t calc_parity(uint8_t b);

void BitReader::output(uint32_t result)
{
  printf("databits:%u, parity:%c, result:0x%08x\r\n", databits,
         parity ? parity : 'N', result);
  uint8_t v = (result >> (CSIZE - databits - !!parity));
  v &= ((1 << databits) - 1);
  if (parity) {
    uint8_t p = calc_parity(v) ^ parity_odd;
    uint8_t pbit = (result >> (CSIZE - 1));
    if (p == pbit)
      rx_fifo.add(v);
  } else {
    rx_fifo.add(v);
  }
}