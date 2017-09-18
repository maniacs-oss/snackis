#ifndef SNABEL_RAT_HPP
#define SNABEL_RAT_HPP

#include "snackis/core/rat.hpp"

namespace snabel{
  struct Exec;
  
  void init_rats(Exec &exe);

  constexpr void pow_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));

    if (y == 0) {
      push(scp, scp.exec.i64_type, (int64_t)0);
      return;
    }
    
    auto v(x);
    
    for (int i = 1; i < abs(y); i++) { v *= x; }
    if (y >= 0) {
      push(scp, scp.exec.i64_type, v);
    } else {
      push(scp, scp.exec.rat_type, Rat(1, abs(v), v < 0));
    }      
  }
}

#endif
