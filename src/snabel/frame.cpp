#include <iostream>
#include "snabel/frame.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Frame::Frame(Scope &scope):
    scope(scope), thread(scope.thread), pc(-1)
  { }

  Frame::~Frame() { }

  void refresh(Frame &frm, Scope &scp) {
    auto &thd(frm.thread);
    frm.pc = thd.pc+1;
    
    frm.stacks.assign(std::next(thd.stacks.begin(), scp.stack_depth),
		      thd.stacks.end());
  }
}
