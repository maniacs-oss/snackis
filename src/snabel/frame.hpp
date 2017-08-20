#ifndef SNABEL_FRAME_HPP
#define SNABEL_FRAME_HPP

#include "snabel/box.hpp"
#include "snabel/env.hpp"

namespace snabel {
  struct Thread;
  
  struct Frame {
    Scope &scope;
    Thread &thread;
    int64_t pc;
    std::deque<Stack> stacks;

    Frame(Scope &scope);
    virtual ~Frame();
  };

  void refresh(Frame &cor, Scope &scp);
}

#endif
