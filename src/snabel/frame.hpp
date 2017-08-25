#ifndef SNABEL_FRAME_HPP
#define SNABEL_FRAME_HPP

#include "snabel/box.hpp"
#include "snabel/env.hpp"

namespace snabel {
  struct Thread;
  
  struct Frame {
    int64_t pc;
    std::deque<Stack> stacks;

    Frame();
    virtual ~Frame();
  };

  void refresh(Frame &frm, Scope &scp);
  void reset(Frame &frm);
}

#endif
