#ifndef SNABEL_THREAD_HPP
#define SNABEL_THREAD_HPP

#include <list>
#include <map>
#include <random>
#include <thread>

#include <poll.h>

#include "snabel/op.hpp"
#include "snabel/poll.hpp"
#include "snabel/refs.hpp"
#include "snabel/scope.hpp"

namespace snabel {
  const int POLL_MAX_FDS(10);

  using Threads = std::list<Thread>;

  struct Thread {
    using Id = std::thread::id;
  
    Exec &exec;
    opt<Id> id;
    Threads::iterator iter;
    std::thread imp;
    PollQueue poll_queue;
    OpSeq ops;
    int64_t pc;
    
    std::deque<Scope> scopes;
    std::deque<Stack> stacks;
    Scope &main;
    LambdaRef lambda;
    
    FileRef _stdin, _stdout;
    size_t io_counter;
    std::default_random_engine random;

    Thread(Exec &exe, opt<Id> id=nullopt);
  };

  void init_threads(Exec &exe);
  
  Scope &curr_scope(Thread &thd);
  const Stack &curr_stack(const Thread &thd);
  Stack &curr_stack(Thread &thd);

  void push(Thread &thd, const Box &val);
  void push(Thread &thd, const Stack &vals);
  Box *peek(Thread &thd);
  opt<Box> try_pop(Thread &thd);
  Box pop(Thread &thd);
  
  Stack &backup_stack(Thread &thd, bool copy=false);
  void reset_stack(Thread &thd, int64_t depth, bool push_result);
  
  Scope &begin_scope(Thread &thd);
  bool end_scope(Thread &thd);

  int64_t find_break_pc(Thread &thd);
  void idle(Thread &thd);
  bool isa(Thread &thd, const Types &x, const Types &y);

  void start(Thread &thd);
  void join(Thread &thd, Scope &scp);
  bool _break(Thread &thd, int64_t depth);
  bool run(Thread &thd, int64_t break_pc=-1); 
  
  constexpr bool isa(Thread &thd, const Type &x, const Type &y) {
    if (&x == &y || (x.raw == y.raw && isa(thd, x.args, y.args))) { return true; }

    for (int64_t i(0); i < x.supers.size(); i++) {
      if (isa(thd, *x.supers[i], y)) { return true; }
    }

    return false;
  }

  constexpr bool isa(Thread &thd, const Box &val, const Type &typ) {
    return isa(thd, *val.type, typ);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const std::thread::id &arg);
}

#endif
