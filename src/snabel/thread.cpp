#include <iostream>
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  static void do_run(Thread *thd) {
    TRY(try_thread);
    run(*thd, false);
  }
  
  Thread::Thread(Exec &exe, Id id):
    exec(exe),
    id(id),
    pc(0),
    main_scope(scopes.emplace_back(*this))
  {
    stacks.emplace_back();
  }

  Scope &curr_scope(Thread &thd) {
    CHECK(!thd.scopes.empty(), _);
    return thd.scopes.back();
  }

  Stack &curr_stack(Thread &thd) {
    CHECK(!thd.stacks.empty(), _);
    return thd.stacks.back();
  }

  const Stack &curr_stack(const Thread &thd) {
    CHECK(!thd.stacks.empty(), _);
    return thd.stacks.back();
  }

  void push(Thread &thd, const Box &val) {
    curr_stack(thd).push_back(val);
  }

  void push(Thread &thd, Type &typ, const Val &val) {
    curr_stack(thd).emplace_back(typ, val);
  }

  void push(Thread &thd, const Stack &vals) {
    std::copy(vals.begin(), vals.end(), std::back_inserter(curr_stack(thd)));
  }

  Box *peek(Thread &thd) {
    auto &s(curr_stack(thd));
    if (s.empty()) { return nullptr; }
    return &s.back();
  }

  opt<Box> try_pop(Thread &thd) {
    auto &s(curr_stack(thd));
    if (s.empty()) { return nullopt; }
    auto res(s.back());
    s.pop_back();
    return res;
  }

  Box pop(Thread &thd) {
    auto &s(curr_stack(thd));
    CHECK(!s.empty(), _);
    auto res(s.back());
    s.pop_back();
    return res;
  }

  Stack &backup_stack(Thread &thd, bool copy) {
    if (copy) { return thd.stacks.emplace_back(thd.stacks.back()); }
    return thd.stacks.emplace_back();
  }
  
  void reset_stack(Thread &thd, int64_t depth, bool push_result) {
    if (thd.stacks.size() > depth) {
      opt<Box> last;
      if (push_result && !thd.stacks.back().empty()) { last = pop(thd); }
      while (thd.stacks.size() > depth) { thd.stacks.pop_back(); }
      if (last) { push(thd, *last); }
    }
  }

  Scope &begin_scope(Thread &thd, bool copy_stack) {
    return thd.scopes.emplace_back(thd.scopes.back());
  }
  
  bool end_scope(Thread &thd) {
    if (thd.scopes.size() < 2) {
      ERROR(Snabel, "Open scope");
      return false;
    }

    thd.scopes.pop_back();
    return true;
  }

  bool isa(Thread &thd, const Types &x, const Types &y) {
    auto i(x.begin()), j(y.begin());

    for (; i != x.end() && j != y.end(); i++, j++) {
      if (!isa(thd, **i, **j)) { return false; }
    }
    
    return i == x.end() && j == y.end();
  }
  
  bool isa(Thread &thd, const Type &x, const Type &y) {
    if (&x == &y || (x.raw == y.raw && isa(thd, x.args, y.args))) { return true; }

    return std::find_if(x.supers.begin(), x.supers.end(), [&thd, &y](auto xs) {
	return isa(thd, *xs, y);
      }) != x.supers.end();
  }

  bool isa(Thread &thd, const Box &val, const Type &typ) {
    return isa(thd, *val.type, typ);
  }
  
  void start(Thread &thd) { thd.imp = std::thread(do_run, &thd); }

  void join(Thread &thd, Scope &scp) {
    thd.imp.join();
    auto &s(curr_stack(thd));
    if (!s.empty()) { push(scp.thread, s); }
    Exec::Lock lock(scp.exec.mutex);
    thd.exec.threads.erase(thd.id);
  }

  bool run(Thread &thd, int64_t break_pc) {
    while (thd.pc < thd.ops.size() && thd.pc != break_pc) {
      auto &op(thd.ops[thd.pc]);
      auto prev_pc(thd.pc);
      
      if (!run(op, curr_scope(thd))) {
	ERROR(Snabel, fmt("Error on line %0: %1 %2",
			  thd.pc, op.imp.name, op.imp.info()));
	return false;
      }
      
      if (thd.pc == prev_pc) { thd.pc++; }
    }

    return true;
  }

  bool run(Thread &thd) {
    begin_scope(thd, true);
    return run(thd, -1);
  }
}
