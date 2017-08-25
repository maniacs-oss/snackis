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
    cor.op_state.clear();
    std::copy(scp.op_state.begin(), scp.op_state.end(),
	      std::inserter(cor.op_state, cor.op_state.end()));
    cor.recalls.clear();
    std::copy(scp.recalls.begin(), scp.recalls.end(),
	      std::back_inserter(cor.recalls));
    auto &thd(scp.thread);
    cor.env.clear();
    
    for (auto &k: scp.env_keys) {
      cor.env.insert(std::make_pair(k, thd.env.at(k)));
    }
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
