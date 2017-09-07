#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/proc.hpp"
#include "snabel/label.hpp"
#include "snabel/list.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  ProcIter::ProcIter(Exec &exe, const ListRef &in):
    Iter(exe, get_iter_type(exe, exe.proc_type)), in(in), i(in->begin())
  { }
  
  opt<Box> ProcIter::next(Scope &scp) {
    if (in->empty()) { return nullopt; }
    if (i == in->end()) { i = in->begin(); }
    auto res(*i);
    if (call(get<ProcRef>(*i), scp, true)) { i++; }
    else { i = in->erase(i); }
    return res;
  }

  static void proc_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.proc_type,
	 std::make_shared<Proc>(get<CoroRef>(args.at(0))));
  }

  static void run_imp(Scope &scp, const Args &args) {
    auto p(get<ProcRef>(args.at(0)));
    while (call(p, scp, true));
  }

  static void list_run_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, exe.proc_type),
	 IterRef(new ProcIter(exe, get<ListRef>(in))));
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
