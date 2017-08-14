#include <iostream>
#include "snabel/compiler.hpp"
#include "snabel/coro.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/parser.hpp"
#include "snackis/core/defer.hpp"
#include "snackis/core/error.hpp"

namespace snabel {
  Coro::Coro(Thread &thd):
    thread(thd), exec(thd.exec), pc(0)
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
  
  bool end_scope(Coro &cor, size_t stack_len) {
    if (cor.scopes.size() < 2) {
      ERROR(Snabel, "Open scope");
      return false;
    }

    restore_stack(cor, stack_len);
    cor.scopes.pop_back();
    return true;
  }

  void reset_scope(Coro &cor, size_t depth) {
    while (cor.scopes.size() > depth) {
      cor.scopes.pop_back();
    }
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
    Exec &exe(cor.exec);
    Exec::Lock lock(exe.mutex);
    Thread &thd(cor.thread);
    
    thd.ops.clear();
    clear_labels(exe);
    rewind(cor);
    size_t lnr(0);
    
    for (auto &ln: parse_lines(in)) {
      if (!ln.empty()) {
	compile(exe, lnr, parse_expr(ln), thd.ops);
      }
       
      lnr++;
    }

    TRY(try_compile);

    while (true) {
      OpSeq out;
      rewind(cor);
      
      for (auto &op: thd.ops) {
	if (!op.prepared && !prepare(op, exe.main_scope)) {
	  goto exit;
	}
	
	cor.pc++;
      }

      cor.pc = 0;
      exe.lambdas.clear();
      for (auto &op: thd.ops) {
	if (!refresh(op, exe.main_scope)) { goto exit; }
	cor.pc++;
      }

      bool done(true);
      exe.lambdas.clear();
      for (auto &op: thd.ops) {
	if (compile(op, exe.main_scope, out)) { done = false; }
      }

      if (done) { goto exit; }
      thd.ops.clear();
      thd.ops.swap(out);
      try_compile.errors.clear();
    }
  exit:
    rewind(cor);
    return try_compile.errors.empty();
  }

  bool run(Coro &cor, bool scope) {
    Thread &thd(cor.thread);
    if (scope) { begin_scope(cor, true); }
    
    while (cor.pc < thd.ops.size()) {
      auto &op(thd.ops[cor.pc]);

      if (!run(op, curr_scope(cor))) {
	ERROR(Snabel, fmt("Error on line %0: %1 %2",
			  cor.pc, op.imp.name, op.imp.info()));
	return false;
      }
      
      cor.pc++;
    }

    return true;
  }
}
