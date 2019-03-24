// Copyright (C) Lars Christensen

#ifndef _lfsr_hpp_
#define _lfsr_hpp_

template <typename T, T mask> struct lfsr {
  lfsr(T init = 1) : reg(init) {}
  T operator()()
  {
    if (reg & 1)
      reg = (reg >> 1) ^ mask;
    else
      reg >>= 1;
    return reg;
  }
  T reg;
};

typedef lfsr<uint16_t, 0xb400> lfsr16;

#endif // _lfsr_hpp_

