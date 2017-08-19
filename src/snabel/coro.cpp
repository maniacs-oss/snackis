#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Scope &scp):
    thread(scp.thread),
    pc(thread.pc+1), stack(curr_stack(thread))
  {
    for (auto &k: scp.env_keys) {
      env.insert(std::make_pair(k, thread.env.at(k)));
    }
  }

  void refresh(Coro &cor, Scope &scp) {
    cor.stack.clear();
    auto &s(curr_stack(cor.thread));
    cor.stack.assign(s.begin(), s.end());

    cor.env.clear();
    for (auto &k: scp.env_keys) {
      cor.env.insert(std::make_pair(k, cor.thread.env.at(k)));
    }
  }
}
