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
    main(fibers.emplace(std::piecewise_construct,
			std::forward_as_tuple(0),
			std::forward_as_tuple(*this, 0)).first->second),
    curr_fiber(&main),
    main_scope(main.scopes.front())
  { }

  void start(Thread &thd) { thd.imp = std::thread(do_run, &thd); }

  void join(Thread &thd, Scope &scp) {
    if (thd.imp.joinable()) { thd.imp.join(); }
    auto &s(curr_stack(*thd.curr_fiber));
    if (!s.empty()) { push(scp.coro, s); }
    Exec::Lock lock(scp.exec.mutex);
    thd.exec.threads.erase(thd.id);
  }

  bool run(Thread &thd, bool scope) {
    if (scope) { begin_scope(*thd.curr_fiber, true); }
    
    while (thd.pc < thd.ops.size()) {
      auto &op(thd.ops[thd.pc]);

      if (!run(op, curr_scope(*thd.curr_fiber))) {
	ERROR(Snabel, fmt("Error on line %0: %1 %2",
			  thd.pc, op.imp.name, op.imp.info()));
	return false;
      }
      
      thd.pc++;
    }

    return true;
  }
}
