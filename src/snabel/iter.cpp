#include <iostream>
#include "snabel/iter.hpp"
#include "snabel/coro.hpp"

namespace snabel {
  Iter::Iter(const Cond &cond, const Box &target):
    cond(cond), target(target)
  { }

  bool next(Iter &iter, Scope &scp) {
    auto nxt(iter.next(iter.cond, scp));
    if (!nxt) { return false; }
    push(scp.coro, *nxt);
    (*iter.target.type->call)(scp, iter.target);
    return true;
  }
}
