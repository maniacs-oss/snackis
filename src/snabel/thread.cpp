#include "snabel/exec.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  static void do_run(Thread *thread) {
    TRY(try_thread);
    run(thread->main);
  }
  
  Thread::Thread(Exec &exe, Id id):
    exec(exe),
    id(id),
    main(fibers.emplace(std::piecewise_construct,
			std::forward_as_tuple(0),
			std::forward_as_tuple(*this, 0)).first->second),
    curr_fiber(&main),
    main_scope(main.scopes.front())
  { }

  void run(Thread &thd) { thd.imp = std::thread(do_run, &thd); }

  void join(Thread &thd, Scope &scp) {
    if (thd.imp.joinable()) { thd.imp.join(); }
    auto &s(curr_stack(*thd.curr_fiber));
    if (!s.empty()) { push(scp.coro, s); }
    Exec::Lock lock(scp.exec.mutex);
    thd.exec.threads.erase(thd.id);
  }
}
