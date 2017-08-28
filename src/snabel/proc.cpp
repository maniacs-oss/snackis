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
  
  static void list_run_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, exe.proc_type),
	 IterRef(new ProcIter(exe, get<ListRef>(in))));
  }

  
  void init_procs(Exec &exe) {
    add_func(exe, "run",
	     {ArgType(get_list_type(exe, exe.proc_type))},
	     list_run_imp);
  }
  
  Proc::Proc(const CoroRef &cor):
    id(uid(cor->target.exec)), coro(cor)
  { }

  bool call(const ProcRef &prc, Scope &scp, bool now) {
    prc->coro->proc = prc;
    return call(prc->coro, scp, now);
  }
}
