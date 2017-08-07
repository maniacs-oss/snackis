#ifndef SNABEL_FIBER_HPP
#define SNABEL_FIBER_HPP

#include "snabel/coro.hpp"
#include "snabel/sym.hpp"

namespace snabel {
  using namespace snackis;

  struct Exec;
  
  struct Fiber: Coro {
    const Sym id;
    
    Fiber(Exec &exe, Sym id);
  };
}

#endif
