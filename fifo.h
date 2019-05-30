#pragma once

#include <cstddef>

/**
 * @brief Generic fixed size FIFO buffer
 *
 * Overflow is discarded.
 */
template <typename T, size_t N> class FIFO
{
public:
  /**
   * Add element to FIFO
   *
   * @param obj Element to be added
   */
  void add(const T &obj)
  {
    size_t next_inp = (inp + 1) % N;
    if (next_inp != inp) {
      fifo[inp] = obj;
      inp = next_inp;
    }
  }

  /**
   * @brief Get and remove element from FIFO
   *
   * @return First output element of FIFO. Value undefined in FIFO is empty.
   */
  const T &get()
  {
    size_t next_outp = (outp + 1) % N;
    size_t curr_outp = outp;
    if (outp != inp)
      outp = next_outp;
    return fifo[curr_outp];
  }

  /**
   * @brief Peek at first output element in FIFO.
   *
   * @return First output element in FIFO. Value undefined if FIFO is empty.
   */
  const T &peek() const { return fifo[outp]; }

  /**
   * @brief Get number of elements in FIFO
   *
   * @return Number of element currently stored in FIFO
   */
  size_t size() const { return (inp + N - outp) % N; }

  /**
   * @brief Check if FIFO is empty
   *
   * @return true if FIFO is empty
   */
  bool empty() const { return inp == outp; }

  /**
   * Clear the FIFO.
   */
  void clear() { inp = outp; }

private:
  /** Current input (write) position in FIFO */
  size_t inp = 0;
  /** Current output (read) position in FIFO */
  size_t outp = 0;
  /** FIFO buffer */
  T fifo[N];
};
