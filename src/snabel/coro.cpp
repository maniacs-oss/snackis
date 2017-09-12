#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/op.hpp"
#include "snabel/proc.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Scope &scp):
    pc(-1), safe_level(-1), target(*scp.target), proc(nullptr), done(false)
  { }

  void refresh(Coro &cor, Scope &scp) {
    auto &thd(scp.thread);
    cor.pc = thd.pc+1;
    cor.safe_level = scp.safe_level;
    cor.stacks.assign(std::next(thd.stacks.begin(), scp.stack_depth),
		      thd.stacks.end());
    
    cor.op_state.swap(scp.op_state);
    cor.env.swap(scp.env);
  }

  bool call(const CoroRef &cor, Scope &scp, bool now) {
    if (cor->done) { return false; };
    scp.coro = cor;
    call(scp, cor->target, now);
    return !cor->done;
  }
}
