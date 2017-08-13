#ifndef SNABEL_FIBER_HPP
#define SNABEL_FIBER_HPP

#include "snabel/coro.hpp"
#include "snabel/sym.hpp"

namespace snabel {
  using namespace snackis;

  struct Fiber: Coro {
    using Id = Sym;
    const Id id;
    
    Fiber(Thread &thd, Id id);
  };
}

#endif
