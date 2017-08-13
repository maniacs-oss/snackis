#ifndef SNABEL_SCOPE_HPP
#define SNABEL_SCOPE_HPP

#include <deque>
#include <map>

#include "snabel/box.hpp"
#include "snabel/func.hpp"
#include "snabel/sym.hpp"
#include "snabel/type.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Coro;
  struct Thread;
  
  using Env = std::map<str, Box>;
  
  struct Scope {
    Coro &coro;
    Thread &thread;
    Exec &exec;
    std::deque<Env> envs;
    Env &root_env;
    
    int64_t return_pc;
    std::deque<int64_t> recall_pcs;
    
    Scope(const Scope &src);
    Scope(Coro &cor);
    const Scope &operator =(const Scope &) = delete;
  };

  Env &curr_env(Scope &scp);
  Env &push_env(Scope &scp);
  void pop_env(Scope &scp);

  Box *find_env(Scope &scp, const str &n);
  Box get_env(Scope &scp, const str &n);
  void put_env(Scope &scp, const str &n, const Box &val);
  bool rem_env(Scope &scp, const str &n);
  void call(Scope &scp, const Label &lbl);

  Thread &start_thread(Scope &scp, const Box &init);
}

#endif
