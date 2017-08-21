#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/env.hpp"
#include "snabel/frame.hpp"

namespace snabel {
  struct Coro: Frame {
    Env env;
    std::deque<Frame> recalls;
    Fiber* fiber;
    
    Coro(Scope &scope);
    virtual ~Coro();
  };

  void refresh(Coro &cor, Scope &scp);
}

#endif
