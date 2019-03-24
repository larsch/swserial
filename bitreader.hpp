// Copyright (C) Lars Christensen

#ifndef _bitreader_hpp_
#define _bitreader_hpp_

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

private:
  /** Number of known bits, including start bit */
  unsigned int bitcount = 0;
  /** Bit length */
  unsigned int bit_length;
  /** Timestamp of last edge */
  uint32_t last_edge_time = 0;
  /** Current byte */
  uint8_t current = 0;
};

#endif // _bitreader_hpp_
