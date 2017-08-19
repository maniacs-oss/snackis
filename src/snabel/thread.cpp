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
  
  void restore_stack(Thread &thd, size_t len) {
    CHECK(!thd.stacks.empty(), _);
    auto prev(thd.stacks.back());
    thd.stacks.pop_back();
    CHECK(!thd.stacks.empty(), _);

    if (len) {
      std::copy((prev.size() <= len)
		? prev.begin()
		: std::next(prev.begin(), prev.size()-len),
		prev.end(),
		std::back_inserter(curr_stack(thd)));
    }
  }

  Scope &begin_scope(Thread &thd, bool copy_stack) {
    return thd.scopes.emplace_back(thd.scopes.back());
  }
  
  bool end_scope(Thread &thd, size_t stack_len) {
    if (thd.scopes.size() < 2) {
      ERROR(Snabel, "Open scope");
      return false;
    }

    thd.scopes.pop_back();
    return true;
  }
  
  void start(Thread &thd) { thd.imp = std::thread(do_run, &thd); }

  void join(Thread &thd, Scope &scp) {
    thd.imp.join();
    auto &s(curr_stack(thd));
    if (!s.empty()) { push(scp.thread, s); }
    Exec::Lock lock(scp.exec.mutex);
    thd.exec.threads.erase(thd.id);
  }

  bool run(Thread &thd, bool scope) {
    if (scope) { begin_scope(thd, true); }
    
    while (thd.pc < thd.ops.size()) {
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
}
