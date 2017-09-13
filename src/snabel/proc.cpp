#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/proc.hpp"
#include "snabel/label.hpp"
#include "snabel/lambda.hpp"
#include "snabel/list.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  static void proc_imp(Scope &scp, const Args &args) {
    push(scp,
	 scp.exec.proc_type,
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
    exe.proc_type.supers.push_back(&exe.any_type);
    exe.proc_type.supers.push_back(&exe.callable_type);
    exe.proc_type.fmt = [](auto &v) { return fmt("Proc(%0)", get<ProcRef>(v)->id); };
    
    exe.proc_type.eq = [](auto &x, auto &y) {
      return get<ProcRef>(x) == get<ProcRef>(y);
    };

    exe.proc_type.call = [](auto &scp, auto &v, bool now) {
      call(get<ProcRef>(v), scp, now);
      return true;
    };

    add_func(exe, "proc", Func::Safe, {ArgType(exe.coro_type)}, proc_imp);
    add_func(exe, "run", Func::Safe, {ArgType(exe.proc_type)}, run_imp);

    add_func(exe, "run", Func::Safe,
	     {ArgType(get_list_type(exe, exe.proc_type))},
	     list_run_imp);

    add_func(exe, "stop", Func::Safe, {ArgType(exe.proc_type)}, stop_imp);
  }
  
  Proc::Proc(const CoroRef &cor):
    id(uid(cor->target->label.exec)), coro(cor)
  { }

  bool call(const ProcRef &prc, Scope &scp, bool now) {
    prc->coro->proc = prc;
    return call(prc->coro, scp, now);
  }
}
