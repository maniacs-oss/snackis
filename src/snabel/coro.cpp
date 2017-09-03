#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/op.hpp"
#include "snabel/proc.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Scope &scp):
    target(*scp.target), start_pc(scp.thread.pc+1), proc(nullptr)
  { }

  void refresh(Coro &cor, Scope &scp) {
    refresh(dynamic_cast<Frame &>(cor), scp);
    cor.op_state.swap(scp.op_state);
    cor.recalls.swap(scp.recalls);
    cor.env.swap(scp.env);
  }

  void reset(Coro &cor) {
    reset(dynamic_cast<Frame &>(cor));
    cor.op_state.clear();
    cor.recalls.clear();
    cor.env.clear();
    cor.pc = cor.start_pc;
    cor.proc.reset();
  }
  
  bool call(const CoroRef &cor, Scope &scp, bool now) {
    scp.coro = cor;
    call(scp, cor->target, now);
    return cor->pc > cor->start_pc;    
  }
}
