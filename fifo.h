#pragma once

#include <cstddef>

template <typename T, size_t N> class FIFO
{
public:
  void add(const T &obj)
  {
    size_t next_inp = (inp + 1) % N;
    if (next_inp != inp) {
      fifo[inp] = obj;
      inp = next_inp;
    }
  }

  const T &get()
  {
    size_t next_outp = (outp + 1) % N;
    size_t curr_outp = outp;
    if (outp != inp)
      outp = next_outp;
    return fifo[curr_outp];
  }

  const T &peek() const { return fifo[outp]; }

  size_t size() const { return (inp + N - outp) % N; }
  bool empty() const { return inp == outp; }
  void clear() { inp = outp; }

private:
  size_t inp = 0;
  size_t outp = 0;
  T fifo[N];
};
