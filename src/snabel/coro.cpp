#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Scope &scp):
    thread(scp.thread)
  {
    refresh(*this, scp);
  }

  void refresh(Coro &cor, Scope &scp) {
    auto &thd(cor.thread);
    cor.pc = thd.pc+1;
    
    cor.stacks.assign(std::next(thd.stacks.begin(),
				thd.stacks.size()-scp.stack_depth),
		     thd.stacks.end());

    cor.env.clear();
    
    for (auto &k: scp.env_keys) {
      cor.env.insert(std::make_pair(k, thd.env.at(k)));
    }
  }
}
