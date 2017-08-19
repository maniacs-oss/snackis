#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Thread &thread):
    Frame(thread)
  { }

  void refresh(Coro &cor, Scope &scp) {
    refresh(dynamic_cast<Frame &>(cor), scp, scp.stack_depth);    
    auto &thd(cor.thread);
    cor.env.clear();
    
    for (auto &k: scp.env_keys) {
      cor.env.insert(std::make_pair(k, thd.env.at(k)));
    }
  }
}
