#include <iostream>
#include "snabel/frame.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Frame::Frame(Thread &thread):
    thread(thread), pc(-1)
  { }

  Frame::~Frame() { }

  void refresh(Frame &frm, Scope &scp, int64_t depth) {
    auto &thd(frm.thread);
    frm.pc = thd.pc+1;
    
    frm.stacks.assign(std::next(thd.stacks.begin(), thd.stacks.size()-depth),
		      thd.stacks.end());
  }
}
