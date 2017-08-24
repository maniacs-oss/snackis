#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/env.hpp"
#include "snabel/frame.hpp"

namespace snabel {
  struct Proc;
  
  struct Coro: Frame {
    Env env;
    std::deque<Frame> recalls;
    Proc* proc;
    
    Coro(Scope &scope);
    Coro(const Coro &) = delete;
    virtual ~Coro();
    const Coro &operator =(const Coro &) = delete;
  };

  void refresh(Coro &cor, Scope &scp);
}

#endif
