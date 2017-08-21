#ifndef SNABEL_FIBER_HPP
#define SNABEL_FIBER_HPP

#include "snabel/box.hpp"
#include "snabel/sym.hpp"
#include "snackis/core/opt.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Coro;
  struct Label;
  struct Scope;
  struct Thread;
  
  struct Fiber {
    using Id = Sym;

    Id id;
    Label &target;
    Coro *coro;
    opt<Box> result;
    
    Fiber(Label &tgt);
  };

  void init(Fiber &fib, Scope &scp);
  bool call(Fiber &fib, Scope &scp, bool now=false);
}

#endif
