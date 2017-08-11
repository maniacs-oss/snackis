#include <iostream>
#include "snabel/compiler.hpp"
#include "snabel/coro.hpp"
#include "snabel/exec.hpp"
#include "snabel/parser.hpp"
#include "snackis/core/defer.hpp"
#include "snackis/core/error.hpp"

namespace snabel {
  Coro::Coro(Exec &exe):
    exec(exe), pc(0)
  {
    begin_scope(*this, false);
  }

  Scope &curr_scope(Coro &cor) {
    CHECK(!cor.scopes.empty(), _);
    return cor.scopes.back();
  }

  Stack &curr_stack(Coro &cor) {
    CHECK(!cor.stacks.empty(), _);
    return cor.stacks.back();
  }

  const Stack &curr_stack(const Coro &cor) {
    CHECK(!cor.stacks.empty(), _);
    return cor.stacks.back();
  }

  void push(Coro &cor, const Box &val) {
    curr_stack(cor).push_back(val);
  }

  void push(Coro &cor, Type &typ, const Val &val) {
    curr_stack(cor).emplace_back(typ, val);
  }

  void push(Coro &cor, const Stack &vals) {
    std::copy(vals.begin(), vals.end(), std::back_inserter(curr_stack(cor)));
  }

  opt<Box> peek(Coro &cor) {
    auto &s(curr_stack(cor));
    if (s.empty()) { return nullopt; }
    return s.back();
  }

  Box pop(Coro &cor) {
    auto &s(curr_stack(cor));
    CHECK(!s.empty(), _);
    auto res(s.back());
    s.pop_back();
    return res;
  }

  Stack &backup_stack(Coro &cor, bool copy) {
    if (cor.stacks.empty() || !copy) {
      return cor.stacks.emplace_back();
    }

    return cor.stacks.emplace_back(cor.stacks.back());
  }
  
  void restore_stack(Coro &cor, size_t len) {
    CHECK(!cor.stacks.empty(), _);
    auto prev(cor.stacks.back());
    cor.stacks.pop_back();
    CHECK(!cor.stacks.empty(), _);

    if (len) {
      std::copy((prev.size() <= len)
		? prev.begin()
		: std::next(prev.begin(), prev.size()-len),
		prev.end(),
		std::back_inserter(curr_stack(cor)));
    }
  }

  Scope &begin_scope(Coro &cor, bool copy_stack) {    
    backup_stack(cor, copy_stack);

    if (cor.scopes.empty()) {
      return cor.scopes.emplace_back(cor);
    }

    return cor.scopes.emplace_back(cor.scopes.back());
  }
  
  void end_scope(Coro &cor, size_t stack_len) {
    CHECK(!cor.scopes.empty(), _);
    restore_stack(cor, stack_len);
    cor.scopes.pop_back();
  }

  void reset_scope(Coro &cor, size_t depth) {
    while (cor.scopes.size() > depth) {
      cor.scopes.pop_back();
    }
  }

  void call(Coro &cor, const Label &lbl){
    cor.returns.push_back(cor.pc);
    jump(cor, lbl);
  }
  
  void jump(Coro &cor, const Label &lbl) {
    reset_scope(cor, lbl.depth);
    cor.pc = lbl.pc;
  }

  void rewind(Coro &cor) {
    while (cor.scopes.size() > 1) { cor.scopes.pop_back(); }
    while (cor.stacks.size() > 1) { cor.stacks.pop_back(); }
    curr_stack(cor).clear();

    Scope &scp(curr_scope(cor));
    while (scp.envs.size() > 1) { scp.envs.pop_back(); }
    
    cor.pc = 0;
  }

  bool compile(Coro &cor, const str &in) {
    cor.ops.clear();
    cor.exec.labels.clear();
    cor.exec.lambdas.clear();
    size_t lnr(0);
    
    for (auto &ln: parse_lines(in)) {
      if (!ln.empty()) {
	compile(cor.exec, lnr, parse_expr(ln), cor.ops);
      }
      
      lnr++;
    }

    begin_scope(cor, false);
    TRY(try_compile);
    
    while (true) {
      OpSeq out;
      
      cor.pc = 0;
      for (auto &op: cor.ops) {
	if (!op.prepared) {
	  prepare(op, curr_scope(cor));
	}
	
	cor.pc++;
      }

      cor.pc = 0;
      for (auto &op: cor.ops) {
	refresh(op, curr_scope(cor));
	cor.pc++;
      }

      cor.pc = 0;
      while (cor.pc < cor.ops.size()) {
	Op &op(cor.ops[cor.pc]);
	if (!trace(op, curr_scope(cor))) { break; }
	cor.pc++;
      }

      bool done(true);
      for (auto &op: cor.ops) {
	if (compile(op, curr_scope(cor), out)) { done = false; }
      }

      if (done) { break; }

      cor.ops.clear();
      cor.ops.swap(out);
      try_compile.errors.clear();
    }

    rewind(cor);
    return try_compile.errors.empty();
  }

  bool run(Coro &cor) {
    begin_scope(cor);
    
    while (cor.pc < cor.ops.size()) {
      if (!run(cor.ops[cor.pc], curr_scope(cor))) { return false; }
      cor.pc++;
    }

    return true;
  }
}
