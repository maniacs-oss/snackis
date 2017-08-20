#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/env.hpp"
#include "snabel/frame.hpp"

namespace snabel {
  struct Coro: Frame {
    Env env;
    Fiber* fiber;
    
    Coro(Thread &thread);
    virtual ~Coro();
  };

  void refresh(Coro &cor, Scope &scp);
  void refresh_env(Coro &cor, Scope &scp);
}

#endif
