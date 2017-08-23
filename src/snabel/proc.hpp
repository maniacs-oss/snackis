#ifndef SNABEL_PROC_HPP
#define SNABEL_PROC_HPP

#include "snabel/box.hpp"
#include "snabel/sym.hpp"
#include "snackis/core/opt.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Coro;
  struct Label;
  struct Scope;
  struct Thread;
  
  struct Proc {
    using Id = Sym;

    Id id;
    Label &target;
    Coro *coro;
    opt<Box> result;
    
    Proc(Label &tgt);
  };

  void init(Proc &prc, Scope &scp);
  bool call(Proc &prc, Scope &scp, bool now=false);
}

#endif
