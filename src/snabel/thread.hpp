#ifndef SNABEL_THREAD_HPP
#define SNABEL_THREAD_HPP

#include <map>
#include <thread>
#include "snabel/scope.hpp"

namespace snabel {
  struct Thread {
    using Id = Sym;
  
    Exec &exec;
    const Id id;
    std::thread imp;
    OpSeq ops;
    int64_t pc;
    Env env;
    
    std::deque<Scope> scopes;
    std::deque<Stack> stacks;
    Scope &main_scope;
    
    Thread(Exec &exe, Id id);
  };

  Scope &curr_scope(Thread &thd);
  const Stack &curr_stack(const Thread &thd);
  Stack &curr_stack(Thread &thd);

  void push(Thread &thd, const Box &val);
  void push(Thread &thd, Type &typ, const Val &val);
  void push(Thread &thd, const Stack &vals);
  
  Box *peek(Thread &thd);
  opt<Box> try_pop(Thread &thd);
  Box pop(Thread &thd);

  Stack &backup_stack(Thread &thd, bool copy=false);
  void restore_stack(Thread &thd, size_t len=1);
  
  Scope &begin_scope(Thread &thd, bool copy_stack=false);
  bool end_scope(Thread &thd);
  
  void start(Thread &thd);
  void join(Thread &thd, Scope &scp);
  bool run(Thread &thd, bool scope=true); 
}
 
#endif
