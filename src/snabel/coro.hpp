#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include "snabel/env.hpp"
#include "snabel/lambda.hpp"
#include "snabel/op.hpp"

namespace snabel {
  using ExitFunc = func<void (Scope &)>;
  using OnExit = std::vector<ExitFunc>;

  struct Coro {
    int64_t pc, safe_level;
    std::deque<Stack> stacks;
    std::map<int64_t, OpState> op_state;
    Env env;

    LambdaRef target;
    OnExit on_exit;
    bool done;
    
    Coro(Scope &scp);
    Coro(const Coro &) = delete;
    const Coro &operator =(const Coro &) = delete;
  };

  using CoroRef = std::shared_ptr<Coro>;

  void init_coros(Exec &exe);
  void refresh(Coro &cor, Scope &scp);
  bool call(const CoroRef &cor, Scope &scp, bool now);
}

#endif
