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
    opt<Fiber *> fiber;
    
    int64_t stack_depth, return_pc;
    bool push_result;

    std::deque<Frame> recalls;
    std::map<Label *, Coro> coros;
    std::set<str> env_keys;
    
    Scope(Thread &thread);
    Scope(const Scope &src);
    ~Scope();
    const Scope &operator =(const Scope &) = delete;
  };

  void restore_stack(Scope &scp, size_t len=1);
  Box *find_env(Scope &scp, const str &key);
  Box get_env(Scope &scp, const str &key);
  void put_env(Scope &scp, const str &key, const Box &val);
  bool rem_env(Scope &scp, const str &key);
  void reset_stack(Scope &scp);
  Coro &add_coro(Scope &scp, Label &tgt);
  Coro *find_coro(Scope &scp, Label &tgt);
  void jump(Scope &scp, const Label &lbl);
  void call(Scope &scp, const Label &lbl);
  bool yield(Scope &scp, Label &tgt);
  void recall_return(Scope &scp);

  Thread &start_thread(Scope &scp, const Box &init);
}

#endif
