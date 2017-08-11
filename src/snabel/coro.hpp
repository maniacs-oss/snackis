#ifndef SNABEL_CORO_HPP
#define SNABEL_CORO_HPP

#include <deque>

#include "snabel/op.hpp"
#include "snabel/scope.hpp"

namespace snabel {
  struct Exec;
  
  struct Coro {
    Exec &exec;
    OpSeq ops;
    int64_t pc;
    
    std::deque<Scope> scopes;
    std::deque<Stack> stacks;
    std::deque<int64_t> returns;
    
    Coro(Exec &exe);
    Coro(const Coro &) = delete;
    const Coro &operator =(const Coro &) = delete;
  };

  Scope &curr_scope(Coro &cor);
  const Stack &curr_stack(const Coro &cor);
  Stack &curr_stack(Coro &cor);

  void push(Coro &cor, const Box &val);
  void push(Coro &cor, Type &typ, const Val &val);
  void push(Coro &cor, const Stack &vals);
  
  opt<Box> peek(Coro &cor);
  Box pop(Coro &cor);

  Stack &backup_stack(Coro &cor, bool copy=false);
  void restore_stack(Coro &cor, size_t len=1);
  
  Scope &begin_scope(Coro &cor, bool copy_stack=false);
  void end_scope(Coro &cor, size_t stack_len=1);
  void reset_scope(Coro &cor, size_t depth);

  void call(Coro &cor, const Label &lbl);
  void jump(Coro &cor, const Label &lbl);
  void rewind(Coro &cor);  

  bool compile(Coro &cor, const str &in);
  bool run(Coro &cor); 
}

#endif
