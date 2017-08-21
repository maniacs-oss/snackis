#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Scope &scope):
    Frame(scope), fiber(nullptr)
  { }

  Coro::~Coro() {
    if (fiber) { fiber->coro = nullptr; }
  }
  
  void refresh(Coro &cor, Scope &scp) {
    refresh(dynamic_cast<Frame &>(cor), scp);
    cor.recalls.clear();
    std::copy(scp.recalls.begin(), scp.recalls.end(),
	      std::back_inserter(cor.recalls));
    auto &thd(cor.thread);
    cor.env.clear();
    
    for (auto &k: scp.env_keys) {
      cor.env.insert(std::make_pair(k, thd.env.at(k)));
    }

  }

}
