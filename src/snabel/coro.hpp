#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/box.hpp"
#include "snabel/env.hpp"

namespace snabel {
  struct Thread;
  
  struct Coro {
    Thread &thread;
    int64_t pc;
    Stack stack;
    Env env;

    Coro(Scope &Scp);
  };

  void refresh(Coro &cor, Scope &scp);

}

#endif
