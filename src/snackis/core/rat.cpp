#include <iostream>
#include "snackis/core/rat.hpp"

namespace snackis {
  Rat::Rat(uint64_t num, uint64_t div, bool neg):
    num(num), div(num ? div : 1), neg(neg)
  {
    auto d(gcd(num, div));

    if (d > 1) {
      this->num /= d;
      this->div /= d;
    }
  }

  Rat operator +(const Rat &x, const Rat &y) {
    auto d(x.div * y.div);
    auto xn(x.num * y.div), yn(y.num * x.div);
    if (x.neg == y.neg) { return Rat(xn + yn, d, x.neg); }
    if (x.neg) { return Rat((yn > xn) ? yn-xn : xn-yn, d, xn > yn); }
    return Rat((yn > xn) ? yn-xn : xn-yn, d, yn > xn);
  }
  
  Rat operator -(const Rat &x, const Rat &y) {
    auto d(x.div * y.div);
    auto xn(x.num * y.div), yn(y.num * x.div);
    if (!x.neg && !y.neg) { return Rat((yn > xn) ? yn-xn : xn-yn, d, yn > xn); }
    if (x.neg != y.neg) { return Rat(xn + yn, d, !y.neg); }
    return Rat((yn > xn) ? yn-xn : xn-yn, d, xn > yn);
  }
  
  Rat operator *(const Rat &x, const Rat &y) {
    return Rat(x.num*y.num, x.div*y.div, x.neg != y.neg);
  }
  
  Rat operator /(const Rat &x, const Rat &y){
    return Rat(x.num*y.div, x.div*y.num, x.neg != y.neg);
  }

  int64_t trunc(const Rat &r) {
    if (r.div == 1) { return r.num; }
    auto res(r.num / r.div);
    return r.neg ? -res : res;
  }

  Rat frac(const Rat &r) {
    return Rat(r.num % r.div, r.div, r.neg);
  }

  template <>
  str fmt_arg(const Rat &arg) {
    if (!arg.num) { return "0"; }
    return fmt("%0%1/%2", arg.neg ? "-" : "", arg.num, arg.div);
  }
}
