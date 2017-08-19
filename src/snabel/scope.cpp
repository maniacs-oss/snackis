#include <iostream>
#include "snabel/scope.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snackis/core/error.hpp"

namespace snabel {  
  Scope::Scope(Thread &thd):
    thread(thd), exec(thread.exec), return_pc(-1)
  { }

  Scope::Scope(const Scope &src):
    thread(src.thread), exec(src.exec), return_pc(-1)
  { }

  Scope::~Scope() {
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

  void jump(Scope &scp, const Label &lbl) {
    if (lbl.yield_tag) {
      yield(scp, *lbl.yield_tag);
    } else {
      if (lbl.recall) { scp.recall_pcs.push_back(scp.thread.pc+1); }
      scp.thread.pc = lbl.pc;
    }
  }

  void call(Scope &scp, const Label &lbl) {
    CHECK(scp.return_pc == -1, _);
    scp.return_pc = scp.thread.pc+1;
    jump(scp, lbl);
  }

  bool yield(Scope &scp, Sym tag) {
    Thread &thd(scp.thread);
    auto yield_pc(scp.thread.pc+1);
    auto yield_stack(curr_stack(thd));

    if (scp.recall_pcs.empty()) {
      if (!end_scope(thd, 1)) { return false; }
      auto &ret_scp(curr_scope(thd));
      
      if (ret_scp.return_pc == -1) {
	ERROR(Snabel, "Missing return pc");
	return false;
      }

      scp.thread.pc = ret_scp.return_pc;
      ret_scp.return_pc = -1;
    } else {
      scp.thread.pc = scp.recall_pcs.back();
      scp.recall_pcs.pop_back();
    }

    auto &prev_scp(curr_scope(thd));
    auto fnd(prev_scp.coros.find(tag));
    
    if (fnd == prev_scp.coros.end()) {
      prev_scp.coros.emplace(std::piecewise_construct,
			     std::forward_as_tuple(tag),
			     std::forward_as_tuple(yield_pc, yield_stack));
    } else {
      fnd->second.first = yield_pc;
      fnd->second.second.assign(yield_stack.begin(), yield_stack.end());
    }

    return true;
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
