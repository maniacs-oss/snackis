#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/env.hpp"
#include "snabel/frame.hpp"

namespace snabel {
  struct Proc;
  
  struct Coro: Frame {
    Env env;
    std::deque<Frame> recalls;
    Label &target;
    int64_t start_pc;
    ProcRef proc;
    
    Coro(Scope &scp);
    Coro(const Coro &) = delete;
    const Coro &operator =(const Coro &) = delete;
  };
  
  void refresh(Coro &cor, Scope &scp);
  void reset(Coro &cor);
  bool call(const CoroRef &cor, Scope &scp, bool now);
}

#endif
