#ifndef SNABEL_THREAD_HPP
#define SNABEL_THREAD_HPP

#include <map>
#include <random>
#include <thread>

#include "snabel/op.hpp"
#include "snabel/scope.hpp"

namespace snabel {
  struct Thread {
    using Id = Uid;
  
    Exec &exec;
    const Id id;
    std::thread imp;
    OpSeq ops;
    int64_t pc;
    Env env;
    std::default_random_engine random;
    
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
  void reset_stack(Thread &thd, int64_t depth, bool push_result);
  
  Scope &begin_scope(Thread &thd, bool copy_stack=false);
  bool end_scope(Thread &thd);
  
  bool isa(Thread &thd, const Types &x, const Types &y);
  bool isa(Thread &thd, const Type &x, const Type &y);
  bool isa(Thread &thd, const Box &val, const Type &typ);

  void start(Thread &thd);
  void join(Thread &thd, Scope &scp);
  bool run(Thread &thd, int64_t break_pc); 
  bool run(Thread &thd); 
}
 
#endif
