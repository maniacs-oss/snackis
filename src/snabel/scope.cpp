#include <iostream>
#include "snabel/coro.hpp"
#include "snabel/scope.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snackis/core/error.hpp"

namespace snabel {  
  Scope::Scope(const Scope &src):
    coro(src.coro), thread(src.thread), exec(src.exec),
    envs(src.envs),
    root_env(envs.front()),
    return_pc(-1)
  { }

  Scope::Scope(Coro &cor):
    coro(cor), thread(cor.thread), exec(thread.exec),
    root_env(push_env(*this)),
    return_pc(-1)
  { }

  Env &curr_env(Scope &scp) {
    CHECK(!scp.envs.empty(), _);
    return scp.envs.back();
  }

  Env &push_env(Scope &scp) {
    if (scp.envs.empty()) {
      return scp.envs.emplace_back(Env());
    }

    return scp.envs.emplace_back(Env(curr_env(scp)));
  }
  
  void pop_env(Scope &scp) {
    CHECK(!scp.envs.empty(), _);
    scp.envs.pop_back();
    CHECK(!scp.envs.empty(), _);
  }

  Box *find_env(Scope &scp, const str &n) {
    auto &env(curr_env(scp));
    auto fnd(env.find(n));
    if (fnd == env.end()) { return nullptr; }
    return &fnd->second;
  }

  Box get_env(Scope &scp, const str &n) {
    auto fnd(find_env(scp, n));
    CHECK(fnd, _);
    return *fnd;
  }

  void put_env(Scope &scp, const str &n, const Box &val, bool force) {
    auto &env(curr_env(scp));
    auto fnd(env.find(n));

    if (fnd != env.end() && !force) {
      ERROR(Snabel, fmt("'%0' is already bound to: %1", n, fnd->second));
    }

    if (fnd == env.end()) {
      env.emplace(std::piecewise_construct,
		  std::forward_as_tuple(n),
		  std::forward_as_tuple(val));
    } else {
      fnd->second = val;
    }
  }

  bool rem_env(Scope &scp, const str &n) {
    return curr_env(scp).erase(n) == 1;
  }

  void call(Scope &scp, const Label &lbl) {
    Coro &cor(scp.coro);
    CHECK(scp.return_pc == -1, _);
    scp.return_pc = cor.pc;
    jump(cor, lbl);
  }

  Thread &start_thread(Scope &scp, const Box &init) {
    Coro &cor(scp.coro);
    Thread &thd(scp.thread);
    Exec &exe(scp.exec);
    
    Thread::Id id(gensym(exe));
    Thread &t(exe.threads.emplace(std::piecewise_construct,
				  std::forward_as_tuple(id),
				  std::forward_as_tuple(exe, id)).first->second);
    auto &s(curr_stack(cor));
    std::copy(s.begin(), s.end(), std::back_inserter(curr_stack(t.main)));

    auto &e(curr_env(scp));
    auto &te(t.main_scope.root_env);
    std::copy(e.begin(), e.end(), std::inserter(te, te.end()));

    std::copy(thd.ops.begin(), thd.ops.end(), std::back_inserter(t.ops));
    t.main.pc = t.ops.size();
    
    if ((*init.type->call)(curr_scope(t.main), init)) {
      run(t.main, false);
    } else {
      ERROR(Snabel, "Failed calling thread init");
    }
    
    return t;
  }
}
