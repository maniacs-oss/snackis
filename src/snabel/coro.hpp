#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/env.hpp"
#include "snabel/op.hpp"

namespace snabel {
  struct Proc;
  
  struct Coro {
    int64_t pc, safe_level;
    std::deque<Stack> stacks;
    std::map<int64_t, OpState> op_state;
    Env env;

    LambdaRef target;
    ProcRef proc;
    bool done;
    
    Coro(Scope &scp);
    Coro(const Coro &) = delete;
    const Coro &operator =(const Coro &) = delete;
  };
  
  void refresh(Coro &cor, Scope &scp);
  bool call(const CoroRef &cor, Scope &scp, bool now);
}

#endif
