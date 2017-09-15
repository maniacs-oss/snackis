#include <iostream>

#include "snabel/coro.hpp"
#include "snabel/exec.hpp"
#include "snabel/lambda.hpp"
#include "snabel/list.hpp"
#include "snabel/op.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Coro::Coro(Scope &scp):
    pc(-1), safe_level(-1), target(scp.thread.lambda), done(false)
  { }

  static void run_imp(Scope &scp, const Args &args) {
    auto p(get<CoroRef>(args.at(0)));
    while (call(p, scp, true));
  }

  static void list_run_imp(Scope &scp, const Args &args) {
    auto ps(get<ListRef>(args.at(0)));
    bool done(false);
    
    while (!done) {
      done = true;

      for (auto i(ps->begin()); i != ps->end();) {
	if (call(get<CoroRef>(*i), scp, true)) {
	  done = false;
	  i++;
	} else {
	  i = ps->erase(i);
	}
      }
    }
  }

  static void stop_imp(Scope &scp, const Args &args) {
    get<CoroRef>(args.at(0))->done = true;
  }

  void init_coros(Exec &exe) {
    exe.coro_type.supers.push_back(&exe.any_type);
    exe.coro_type.supers.push_back(&exe.callable_type);

    exe.coro_type.fmt = [](auto &v) {
      auto &l(get<CoroRef>(v)->target->label);
      return fmt("Coro(%0:%1)", name(l.tag), l.pc);
    };
    
    exe.coro_type.eq = [](auto &x, auto &y) {
      return get<CoroRef>(x) == get<CoroRef>(y);
    };

    exe.coro_type.call = [](auto &scp, auto &v, bool now) {
      call(get<CoroRef>(v), scp, now);
      return true;
    };

    add_func(exe, "run", Func::Safe, {ArgType(exe.coro_type)}, run_imp);

    add_func(exe, "run", Func::Safe,
	     {ArgType(get_list_type(exe, exe.coro_type))},
	     list_run_imp);

    add_func(exe, "stop", Func::Safe, {ArgType(exe.coro_type)}, stop_imp);
  }

  void refresh(Coro &cor, Scope &scp) {
    auto &thd(scp.thread);
    cor.pc = thd.pc+1;
    cor.safe_level = scp.safe_level;
    cor.stacks.assign(std::next(thd.stacks.begin(), scp.stack_depth),
		      thd.stacks.end());
    
    cor.op_state.swap(scp.op_state);
    cor.env.swap(scp.env);
  }

  bool call(const CoroRef &cor, Scope &scp, bool now) {
    if (cor->done) { return false; };
    scp.coro = cor;
    call(cor->target, scp, now);
    return !cor->done;
  }
}
