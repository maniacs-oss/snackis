#ifndef SNABEL_FIBER_HPP
#define SNABEL_FIBER_HPP

#include "snabel/sym.hpp"

namespace snabel {
  struct Coro;
  struct Label;
  struct Scope;
  struct Thread;
  
  struct Fiber {
    using Id = Sym;

    Thread &thread;
    Id id;
    Label &target;
    Coro *coro;
    
    Fiber(Thread &thd, Id id, Label &tgt);
  };

  void init(Fiber &fib, Scope &scp);
  void call(Fiber &fib, Scope &scp);
}

#endif
