#include <iostream>
#include "snabel/iter.hpp"
#include "snabel/coro.hpp"

namespace snabel {
  Iter::Iter(const Box &target, Next next):
    target(target), next(next)
  { }

  bool next(Iter &iter, Scope &scp) {
    auto nxt(iter.next(scp));
    if (!nxt) { return false; }
    push(scp.coro, *nxt);
    (*iter.target.type->call)(scp, iter.target);
    return true;
  }
}
