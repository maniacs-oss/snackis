#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/op.hpp"
#include "snabel/proc.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Scope &scp):
    target(*scp.target), proc(nullptr), done(false)
  { }

  void refresh(Coro &cor, Scope &scp) {
    refresh(dynamic_cast<Frame &>(cor), scp);
    cor.op_state.swap(scp.op_state);
    cor.recalls.swap(scp.recalls);
    cor.env.swap(scp.env);
  }

  bool call(const CoroRef &cor, Scope &scp, bool now) {
    if (cor->done) { return false; };
    scp.coro = cor;
    call(scp, cor->target, now);
    return !cor->done;
  }
}
