#include <iostream>
#include "snabel/frame.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Frame::Frame():
    pc(-1)
  { }

  Frame::~Frame() { }

  void refresh(Frame &frm, Scope &scp) {
    auto &thd(scp.thread);
    frm.pc = thd.pc+1;
    
    frm.stacks.assign(std::next(thd.stacks.begin(), scp.stack_depth),
		      thd.stacks.end());
  }

  void reset(Frame &frm) {
    frm.pc = -1;
    frm.stacks.clear();
  }			 
}
