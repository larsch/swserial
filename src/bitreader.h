// Copyright (C) Lars Christensen

#ifndef _bitreader_h_
#define _bitreader_h_

#include "fifo.h"
#include <cstdint>

/**
 * @brief Bit reader.
 *
 * Detects start bit and databits in from a series of rising/falling edges with
 * timestamps.
 */
class BitReader
{
public:
  /**
   * Construct bit BitReader
   *
   * @param bitlen Length of bit
   */
  BitReader(unsigned int bitlen = 100) : bit_length(bitlen) {}

  /**
   * @brief Set bit length
   *
   * @param bitlen New bit length
   */
  void setBitLength(int bitlen) { bit_length = bitlen; }

  /**
   * @brief Handle bit edge
   *
   * Called when an edge is detected with the current timestamp.
   *
   * @param time Timestamp of edge
   * @param level New level (0 if falling edge, 1 if rising)
   */
  void edge(uint32_t time, int level);

  /**
   * @brief Check for timeout of current byte. Needs to be called to detect end
   * of byte if the MSB are high and there's no stop-bit edge.
   *
   * @param time Current time
   */
  void check(uint32_t time);

  FIFO<uint8_t, 32> rx_fifo;

  void begin(int data, char par)
  {
    databits = data;
    parity = par;
    parity_odd = (par == 'O' ? 1 : 0);
    expected_bits = 1 + databits + !!par;
  }

protected:
  void output(uint32_t result);

  /** Bit length */
  unsigned int bit_length;
  /** Expected number of bits, including start & parity */
  uint8_t expected_bits = 9;
  /** Number of known bits, including start bit */
  unsigned int bitcount = 0;
  /** Timestamp of last edge */
  uint32_t last_edge_time = 0;
  /** Current byte & parity (in MSB) */
  unsigned int current = 0;

  unsigned int databits = 8;
  unsigned int parity = 0;
  unsigned int parity_odd = 0;
};

#endif // _bitreader_h_
