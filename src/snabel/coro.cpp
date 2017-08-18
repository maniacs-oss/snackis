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
    thread(thd), exec(thd.exec), main_scope(scopes.emplace_back(*this))
  {
    stacks.emplace_back();
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

  Box *peek(Coro &cor) {
    auto &s(curr_stack(cor));
    if (s.empty()) { return nullptr; }
    return &s.back();
  }

  opt<Box> try_pop(Coro &cor) {
    auto &s(curr_stack(cor));
    if (s.empty()) { return nullopt; }
    auto res(s.back());
    s.pop_back();
    return res;
  }

  Box pop(Coro &cor) {
    auto &s(curr_stack(cor));
    CHECK(!s.empty(), _);
    auto res(s.back());
    s.pop_back();
    return res;
  }

  Stack &backup_stack(Coro &cor, bool copy) {
    if (copy) {
      return cor.stacks.emplace_back(cor.stacks.back());
    }
    
    return cor.stacks.emplace_back();
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
    return cor.scopes.emplace_back(cor.scopes.back());
  }
  
  bool end_scope(Coro &cor, size_t stack_len) {
    if (cor.scopes.size() < 2) {
      ERROR(Snabel, "Open scope");
      return false;
    }

    cor.scopes.pop_back();
    return true;
  }

  void jump(Coro &cor, const Label &lbl) {
    if (lbl.recall) {
      auto &scp(curr_scope(cor));
      scp.recall_pcs.push_back(scp.thread.pc+1);
    }
    
    cor.thread.pc = lbl.pc;
  }
}
