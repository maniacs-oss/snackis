#ifndef SNABEL_SCOPE_HPP
#define SNABEL_SCOPE_HPP

#include <deque>
#include <map>

#include "snabel/box.hpp"
#include "snabel/coro.hpp"
#include "snabel/env.hpp"
#include "snabel/func.hpp"
#include "snabel/sym.hpp"
#include "snabel/type.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Thread;
  
  struct Scope {
    Thread &thread;
    Exec &exec;
    
    size_t stack_depth;
    int64_t return_pc;
    std::deque<int64_t> recall_pcs;
    std::map<Sym, Coro> coros;
    std::set<str> env_keys;
    
    Scope(Thread &thread);
    Scope(const Scope &src);
    ~Scope();
    const Scope &operator =(const Scope &) = delete;
  };

  Box *find_env(Scope &scp, const str &key);
  Box get_env(Scope &scp, const str &key);
  void put_env(Scope &scp, const str &key, const Box &val);
  bool rem_env(Scope &scp, const str &key);
  void jump(Scope &scp, const Label &lbl);
  void call(Scope &scp, const Label &lbl);
  bool yield(Scope &scp, Sym tag);

  Thread &start_thread(Scope &scp, const Box &init);
}

#endif
