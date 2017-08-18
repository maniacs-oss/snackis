#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/scope.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snackis/core/error.hpp"

namespace snabel {  
  Scope::Scope(const Scope &src):
    coro(src.coro), thread(src.thread), exec(src.exec), return_pc(-1)
  { }

  Scope::Scope(Coro &cor):
    coro(cor), thread(cor.thread), exec(thread.exec), return_pc(-1)
  { }

  Scope::~Scope() {
    for (auto &k: env_keys) { coro.env.erase(k); }
  }

  Box *find_env(Scope &scp, const str &key) {
    auto &env(scp.coro.env);
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
    auto &env(scp.coro.env);
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

    if (&scp != &scp.coro.main_scope) { scp.env_keys.insert(key); }
  }

  bool rem_env(Scope &scp, const str &key) {
    if (&scp != &scp.coro.main_scope) {
      auto fnd(scp.env_keys.find(key));
      if (fnd == scp.env_keys.end()) { return false; }
      scp.env_keys.erase(fnd);
    }
    
    scp.coro.env.erase(key);
    return true;
  }

  void call(Scope &scp, const Label &lbl) {
    Coro &cor(scp.coro);
    CHECK(scp.return_pc == -1, _);
    scp.return_pc = scp.thread.pc;
    jump(cor, lbl);
  }

  Thread &start_thread(Scope &scp, const Box &init) {
    Coro &cor(scp.coro);
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
    
    auto &s(curr_stack(cor));
    std::copy(s.begin(), s.end(), std::back_inserter(curr_stack(t->main)));

    auto &e(scp.coro.env);
    auto &te(t->main.env);
    std::copy(e.begin(), e.end(), std::inserter(te, te.end()));

    std::copy(thd.ops.begin(), thd.ops.end(), std::back_inserter(t->ops));
    t->pc = t->ops.size();
    
    if ((*init.type->call)(curr_scope(t->main), init)) {
      start(*t);
    } else {
      ERROR(Snabel, "Failed initializing thread");
    }
    
    return *t;
  }
}
