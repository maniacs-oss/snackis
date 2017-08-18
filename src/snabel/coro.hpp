#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include <deque>

#include "snabel/op.hpp"
#include "snabel/scope.hpp"

namespace snabel {
  struct Thread;

  struct Coro {
    Thread &thread;
    Exec &exec;
    Env env;
    
    std::deque<Scope> scopes;
    std::deque<Stack> stacks;
    Scope &main_scope;
    
    Coro(Thread &thread);
    Coro(const Coro &) = delete;
    const Coro &operator =(const Coro &) = delete;
  };

  Scope &curr_scope(Coro &cor);
  const Stack &curr_stack(const Coro &cor);
  Stack &curr_stack(Coro &cor);

  void push(Coro &cor, const Box &val);
  void push(Coro &cor, Type &typ, const Val &val);
  void push(Coro &cor, const Stack &vals);
  
  Box *peek(Coro &cor);
  opt<Box> try_pop(Coro &cor);
  Box pop(Coro &cor);

  Stack &backup_stack(Coro &cor, bool copy=false);
  void restore_stack(Coro &cor, size_t len=1);
  
  Scope &begin_scope(Coro &cor, bool copy_stack=false);
  bool end_scope(Coro &cor, size_t stack_len=1);
  void reset_scope(Coro &cor, size_t depth);

  void jump(Coro &cor, const Label &lbl);
}

#endif
