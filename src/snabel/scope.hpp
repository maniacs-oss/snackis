#ifndef SNABEL_SCOPE_HPP
#define SNABEL_SCOPE_HPP

#include <deque>
#include <map>

#include "snabel/box.hpp"
#include "snabel/coro.hpp"
#include "snabel/env.hpp"
#include "snabel/func.hpp"
#include "snabel/op.hpp"
#include "snabel/sym.hpp"
#include "snabel/type.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Thread;
  
  struct Scope {
    using ReturnFunc = func<void ()>;
    
    Thread &thread;
    Exec &exec;
    Scope *parent;

    Label *target;
    CoroRef coro;
    int64_t stack_depth, safe_level, return_pc, recall_pc, break_pc;
    bool push_result;
    
    std::map<int64_t, OpState> op_state;
    std::vector<ReturnFunc> on_return;
    Env env;
    
    Scope(Thread &thread);
    Scope(Scope &prt);
    ~Scope();
    const Scope &operator =(const Scope &) = delete;
  };

  void push(Scope &scp, Type &typ, const Val &val);
  void push(Scope &scp, Type &typ);
  
  void restore_stack(Scope &scp, size_t len=1);
  Box *find_env(Scope &scp, const Sym &key);
  Box *find_env(Scope &scp, const str &key);
  void put_env(Scope &scp, const Sym &key, const Box &val);
  void put_env(Scope &scp, const str &key, const Box &val);
  bool rem_env(Scope &scp, const Sym &key);
  bool rem_env(Scope &scp, const str &key);
  void reset_stack(Scope &scp);
  void jump(Scope &scp, const Label &lbl);
  bool call(Scope &scp, const Label &lbl, bool now=false);
  bool _return(Scope &scp, int64_t depth, bool push_result);
  bool recall(Scope &scp, int64_t depth);
  void recall_return(Scope &scp);
  bool yield(Scope &scp, int64_t depth, bool push_result);

  Thread &start_thread(Scope &scp, const Box &init);
}

#endif
