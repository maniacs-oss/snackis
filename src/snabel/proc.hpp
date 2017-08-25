#ifndef SNABEL_PROC_HPP
#define SNABEL_PROC_HPP

#include "snabel/box.hpp"
#include "snabel/uid.hpp"
#include "snackis/core/opt.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Coro;
  struct Label;
  struct Scope;
  struct Thread;
  
  struct Proc {
    using Id = Uid;

    Id id;
    CoroRef coro;
    
    Proc(const CoroRef &cor);
  };

  void init(Proc &prc, Scope &scp);
  bool call(const ProcRef &prc, Scope &scp, bool now=false);
}

#endif
