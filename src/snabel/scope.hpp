#ifndef SNABEL_SCOPE_HPP
#define SNABEL_SCOPE_HPP

#include <deque>
#include <map>

#include "snabel/box.hpp"
#include "snabel/env.hpp"
#include "snabel/func.hpp"
#include "snabel/sym.hpp"
#include "snabel/type.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Coro;
  struct Thread;
  
  struct Scope {
    Coro &coro;
    Thread &thread;
    Exec &exec;
    
    int64_t return_pc;
    std::deque<int64_t> recall_pcs;
    std::map<Coro *, int64_t> coro_pcs;
    std::set<str> env_keys;
    
    Scope(const Scope &src);
    Scope(Coro &cor);
    ~Scope();
    const Scope &operator =(const Scope &) = delete;
  };

  Box *find_env(Scope &scp, const str &key);
  Box get_env(Scope &scp, const str &key);
  void put_env(Scope &scp, const str &key, const Box &val);
  bool rem_env(Scope &scp, const str &key);
  void call(Scope &scp, const Label &lbl);

  Thread &start_thread(Scope &scp, const Box &init);
}

#endif
