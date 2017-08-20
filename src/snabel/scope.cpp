#include <iostream>
#include "snabel/scope.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snackis/core/error.hpp"

namespace snabel {  
  Scope::Scope(Thread &thd):
    thread(thd),
    exec(thread.exec),
    stack_depth(thread.stacks.size()),
    return_pc(-1),
    push_result(true)
  { }

  Scope::Scope(const Scope &src):
    thread(src.thread),
    exec(src.exec),
    stack_depth(thread.stacks.size()),
    return_pc(-1),
    push_result(true),
    coros(src.coros)
  {}
  
  Scope::~Scope() {
    reset_stack(*this);
    for (auto &k: env_keys) { thread.env.erase(k); }
  }

  Box *find_env(Scope &scp, const str &key) {
    auto &env(scp.thread.env);
    auto fnd(env.find(key));
    if (fnd == env.end()) { return nullptr; }
    return &fnd->second;
  }

  Box get_env(Scope &scp, const str &key) {
    auto fnd(find_env(scp, key));
    CHECK(fnd, _);
    return *fnd;
  }

  void put_env(Scope &scp, const str &key, const Box &val) {
    auto &env(scp.thread.env);
    auto fnd(env.find(key));

    if (fnd != env.end()) {
      ERROR(Snabel, fmt("'%0' is already bound to: %1", key, fnd->second));
    }

    if (fnd == env.end()) {
      env.emplace(std::piecewise_construct,
		  std::forward_as_tuple(key),
		  std::forward_as_tuple(val));
    } else {
      fnd->second = val;
    }

    if (&scp != &scp.thread.main_scope) { scp.env_keys.insert(key); }
  }

  bool rem_env(Scope &scp, const str &key) {
    if (&scp != &scp.thread.main_scope) {
      auto fnd(scp.env_keys.find(key));
      if (fnd == scp.env_keys.end()) { return false; }
      scp.env_keys.erase(fnd);
    }
    
    scp.thread.env.erase(key);
    return true;
  }

  void reset_stack(Scope &scp) {
    reset_stack(scp.thread, scp.stack_depth, scp.push_result);
  }

  Coro *find_coro(Scope &scp, Label &tgt) {
    auto fnd(scp.coros.find(&tgt));
    if (fnd == scp.coros.end()) { return nullptr; }
    return &fnd->second;
  }

  void jump(Scope &scp, const Label &lbl) {
    auto &thd(scp.thread);
    
    if (lbl.yield_target) {
      yield(scp, *lbl.yield_target);
    } else {      
      if (lbl.recall) {
	auto &frm(scp.recalls.emplace_back(thd));
	refresh(frm, scp);
	reset_stack(thd, scp.stack_depth, true);
      }
      
      thd.pc = lbl.pc;
    }
  }

  void call(Scope &scp, const Label &lbl) {
    CHECK(scp.return_pc == -1, _);
    scp.return_pc = scp.thread.pc+1;
    jump(scp, lbl);
  }

  bool yield(Scope &scp, Label &tgt) {
    Thread &thd(scp.thread);
    auto &prev_scp(*std::next(thd.scopes.rbegin()));
    auto fnd(prev_scp.coros.find(&tgt));
    
    if (fnd == prev_scp.coros.end()) {
      auto &cor(prev_scp.coros.emplace(std::piecewise_construct,
				       std::forward_as_tuple(&tgt),
				       std::forward_as_tuple(thd)).first->second);
      refresh(cor, scp);
      if (cor.fiber && scp.recalls.empty()) { scp.push_result = false; }
    } else {
      auto &cor(fnd->second);
      refresh(cor, scp);
      if (cor.fiber && scp.recalls.empty()) { scp.push_result = false; }
    }
    
    if (scp.recalls.empty()) {
      if (!end_scope(thd)) { return false; }
      
      if (prev_scp.return_pc == -1) {
	ERROR(Snabel, "Missing return pc");
	return false;
      }

      scp.thread.pc = prev_scp.return_pc;
      prev_scp.return_pc = -1;
    } else {
      recall_return(scp);
    }

    return true;
  }

  void recall_return(Scope &scp) {
    auto &thd(scp.thread);
    auto &frm(scp.recalls.back());
    thd.pc = frm.pc;
    opt<Box> last;
    if (!thd.stacks.back().empty()) { last = pop(thd); }    
    std::copy(frm.stacks.begin(), frm.stacks.end(),
	      std::back_inserter(thd.stacks));
    if (last) { push(thd, *last); }
    scp.recalls.pop_back();
  }

  Thread &start_thread(Scope &scp, const Box &init) {
    Thread &thd(scp.thread);
    Exec &exe(scp.exec);
    
    Thread::Id id(gensym(exe));

    Thread *t(nullptr);

    {
      Exec::Lock lock(exe.mutex);
      t = &exe.threads.emplace(std::piecewise_construct,
			       std::forward_as_tuple(id),
			       std::forward_as_tuple(exe, id)).first->second;
    }
    
    auto &s(curr_stack(thd));
    std::copy(s.begin(), s.end(), std::back_inserter(curr_stack(*t)));

    auto &e(thd.env);
    auto &te(t->env);
    std::copy(e.begin(), e.end(), std::inserter(te, te.end()));

    std::copy(thd.ops.begin(), thd.ops.end(), std::back_inserter(t->ops));
    t->pc = t->ops.size();
    
    if ((*init.type->call)(curr_scope(*t), init)) {
      start(*t);
    } else {
      ERROR(Snabel, "Failed initializing thread");
    }
    
    return *t;
  }
}
