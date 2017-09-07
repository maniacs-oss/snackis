#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/proc.hpp"
#include "snabel/label.hpp"
#include "snabel/list.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  static void proc_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.proc_type,
	 std::make_shared<Proc>(get<CoroRef>(args.at(0))));
  }

  static void run_imp(Scope &scp, const Args &args) {
    auto p(get<ProcRef>(args.at(0)));
    while (call(p, scp, true));
  }

  static void list_run_imp(Scope &scp, const Args &args) {
    auto &ps(get<ListRef>(args.at(0)));
    bool done(false);
    
    while (!done) {
      done = true;

      for (auto i(ps->begin()); i != ps->end();) {
	if (call(get<ProcRef>(*i), scp, true)) {
	  done = false;
	  i++;
	} else {
	  i = ps->erase(i);
	}
      }
    }
  }

  static void stop_imp(Scope &scp, const Args &args) {
    get<ProcRef>(args.at(0))->coro->done = true;
  }
  
  void init_procs(Exec &exe) {
    add_func(exe, "proc", {ArgType(exe.coro_type)}, proc_imp);
    add_func(exe, "run", {ArgType(exe.proc_type)}, run_imp);

    add_func(exe, "run",
	     {ArgType(get_list_type(exe, exe.proc_type))},
	     list_run_imp);

    add_func(exe, "stop", {ArgType(exe.proc_type)}, stop_imp);
  }
  
  Proc::Proc(const CoroRef &cor):
    id(uid(cor->target.exec)), coro(cor)
  { }

  bool call(const ProcRef &prc, Scope &scp, bool now) {
    prc->coro->proc = prc;
    return call(prc->coro, scp, now);
  }
}
