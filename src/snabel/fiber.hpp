#ifndef SNABEL_FIBER_HPP
#define SNABEL_FIBER_HPP

#include "snabel/sym.hpp"

namespace snabel {
  struct Label;
  struct Thread;
  
  struct Fiber {
    using Id = Sym;

    Thread &thread;
    Id id;
    Label &target;
    
    Fiber(Thread &thd, Id id, Label &tgt);
  };
}

#endif
