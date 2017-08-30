#ifndef SNACKIS_RAT_HPP
#define SNACKIS_RAT_HPP

#include <cstdint>
#include "snackis/core/fmt.hpp"
#include "snackis/core/str.hpp"

namespace snackis {
  struct Rat {
    uint64_t num, div;
    bool neg;
    
    Rat(uint64_t num, uint64_t div, bool neg=false);
  };

  Rat operator +(const Rat &x, const Rat &y);
  Rat operator -(const Rat &x, const Rat &y);
  Rat operator *(const Rat &x, const Rat &y);
  Rat operator /(const Rat &x, const Rat &y);
  
  int64_t trunc(const Rat &r);
  Rat frac(const Rat &r);

  template <>
  str fmt_arg(const snackis::Rat &arg);

  constexpr bool operator ==(const Rat &x, const Rat &y) {
    return x.num == y.num && x.div == y.div && x.neg == y.neg;
  }

  constexpr bool operator <(const Rat &x, const Rat &y) {
    return x.num*y.div < y.num*x.div; 
  }

  constexpr uint64_t gcd(uint64_t x, uint64_t y) {
    while (x && y) {
      auto z(y);
      y = x % y;
      x = z;
    }

    return x;
  }
}

#endif
