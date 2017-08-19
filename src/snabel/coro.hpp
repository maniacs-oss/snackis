#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/env.hpp"
#include "snabel/frame.hpp"

namespace snabel {
  struct Coro: Frame {
    Env env;

    Coro(Thread &thread);
  };

  void refresh(Coro &cor, Scope &scp);

}

#endif
