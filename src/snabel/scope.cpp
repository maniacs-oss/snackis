#include <iostream>
#include "snabel/scope.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snackis/core/error.hpp"

namespace snabel {  
  Scope::Scope(Thread &thd):
    thread(thd),
    exec(thread.exec),
    parent(nullptr),
    target(nullptr),
    coro(nullptr),
    stack_depth(thread.stacks.size()),
    safe_level(0),
    return_pc(-1),
    recall_pc(-1),
    break_pc(-1),
    push_result(true)
  { }

  Scope::Scope(Scope &prt):
    thread(prt.thread),
    exec(prt.exec),
    parent(&prt),
    target(nullptr),
    coro(nullptr),
    stack_depth(thread.stacks.size()),
    safe_level(prt.safe_level),
    return_pc(-1),
    recall_pc(-1),
    break_pc(-1),
    push_result(true)
  {}
  
  Scope::~Scope() {
    reset_stack(*this);
  }
  
  void restore_stack(Scope &scp, size_t len) {
    auto &thd(scp.thread);
    CHECK(!thd.stacks.empty(), _);
    auto prev(thd.stacks.back());
    thd.stacks.pop_back();
    CHECK(!thd.stacks.empty(), _);

    if (len && ((thd.stacks.size() > scp.stack_depth) || scp.push_result)) {
      std::copy((prev.size() <= len)
		? prev.begin()
		: std::next(prev.begin(), prev.size()-len),
		prev.end(),
		std::back_inserter(curr_stack(thd)));
    }
  }

  Box *find_env(Scope &scp, const Sym &key) {
    auto fnd(scp.env.find(key));
    if (fnd != scp.env.end()) { return &fnd->second; }

    return (scp.parent && scp.parent->safe_level == scp.safe_level)
      ? find_env(*scp.parent, key)
      : nullptr;
  }

  Box *find_env(Scope &scp, const str &key) {
    return find_env(scp, get_sym(scp.exec, key));
  }

  void put_env(Scope &scp, const Sym &key, const Box &val) {
    auto fnd(scp.env.find(key));

    if (fnd == scp.env.end()) {
      scp.env.emplace(std::piecewise_construct,
		      std::forward_as_tuple(key),
		      std::forward_as_tuple(val));
    } else {
      ERROR(Snabel, fmt("Rebinding name: %0", name(key)));
      fnd->second = val;
    }
  }

  void put_env(Scope &scp, const str &key, const Box &val) {
    put_env(scp, get_sym(scp.exec, key), val);
  }
  
  bool rem_env(Scope &scp, const Sym &key) {
    scp.env.erase(key);
    return true;
  }

  bool rem_env(Scope &scp, const str &key) {
    return rem_env(scp, get_sym(scp.exec, key));
  }

  void reset_stack(Scope &scp) {
    reset_stack(scp.thread, scp.stack_depth, scp.push_result);
  }

  void jump(Scope &scp, const Label &lbl) {
    if (lbl.return_depth) {
      _return(scp, lbl.return_depth);
    } else if (lbl.recall_depth) {
      recall(scp, lbl.recall_depth);
    } else if (lbl.yield_depth) {
      yield(scp, lbl.yield_depth);      
    } else if (lbl.break_depth) {
      _break(scp.thread, lbl.break_depth);
    } else {
      scp.thread.pc = lbl.pc;
    }
  }

  void call(Scope &scp, const Label &lbl, bool now) {
    if (scp.return_pc != -1) {
      ERROR(Snabel, fmt("Overwriting return: ", scp.return_pc));
    }
    
    scp.return_pc = scp.thread.pc+1;
    jump(scp, lbl);

    if (now) {
      auto ret_pc(scp.return_pc);
      run(scp.thread, scp.return_pc);
      if (scp.thread.pc == ret_pc) { scp.thread.pc--; }
    }
  }

  bool yield(Scope &scp, int64_t depth) {
    Thread &thd(scp.thread);
    bool dec_pc(false);

    while (depth && thd.scopes.size() > 1) {
      depth--;
      auto &prev_scp(*std::next(thd.scopes.rbegin()));
      CoroRef cor;
      bool new_cor(false);
      
      if (!depth) {
	auto &curr_scp(thd.scopes.back());
	
	if (!curr_scp.target) {
	  ERROR(Snabel, "Missing yield target");
	  return false;
	}
	
	cor = prev_scp.coro;
	
	if (cor) {
	  cor->proc.reset();
	  prev_scp.coro = nullptr;
	} else {
	  cor = std::make_shared<Coro>(curr_scp);
	  new_cor = true;
	}

	refresh(*cor, curr_scp);
	if (dec_pc) { cor->pc--; }
	
	if (prev_scp.return_pc == -1) {
	  ERROR(Snabel, "Missing return pc");
	  return false;
	}	
      }

      dec_pc = true;
      thd.pc = prev_scp.return_pc;
      prev_scp.return_pc = -1;
      prev_scp.coro.reset();
      if (!end_scope(thd)) { return false; }
      if (new_cor) { push(thd, thd.exec.coro_type, cor); }
    }
    
    return true;
  }

  bool _return(Scope &scp, int64_t depth) {
    auto &thd(scp.thread);
    
    while (thd.scopes.size() > 1 && depth) {
      depth--;

      if (!depth) {
	auto &ps(*std::next(thd.scopes.rbegin()));	  
	
	if (ps.return_pc == -1) {
	  ERROR(Snabel, "Missing return pc");
	  return false;
	}
	
	end_scope(thd);
	thd.pc = ps.return_pc;
	ps.return_pc = -1;
	if (ps.coro) { ps.coro->done = true; }
	ps.coro.reset();
	break;
      }
      
      end_scope(thd);
    }

    return true;
  }

  bool recall(Scope &scp, int64_t depth) {
    auto &thd(scp.thread);
    
    while (thd.scopes.size() > 1) {
      auto &s(thd.scopes.back());
      depth--;

      if (!depth) {
	if (s.recall_pc == -1) {
	  ERROR(Snabel, "Missing recall pc");
	  return false;
	}
	
	auto &thd(s.thread);
	reset_stack(thd, s.stack_depth, true);
	s.env.clear();
	thd.pc = s.recall_pc;
	break;
      }

      end_scope(thd);
    }

    return true;
  }

  Thread &start_thread(Scope &scp, const Box &init) {
    Thread &thd(scp.thread);
    Exec &exe(scp.exec);
    Thread::Id id(uid(exe));
    Thread *t(nullptr);

    {
      Exec::Lock lock(exe.mutex);
      t = &exe.threads.emplace(std::piecewise_construct,
			       std::forward_as_tuple(id),
			       std::forward_as_tuple(exe, id)).first->second;
    }
    
    auto &stk(curr_stack(thd));
    std::copy(stk.begin(), stk.end(), std::back_inserter(curr_stack(*t)));
    auto &te(t->main.env);
    
    for (auto &s: thd.scopes) {
      auto &e(s.env);
      std::copy(e.begin(), e.end(), std::inserter(te, te.end()));
    }
    
    std::copy(thd.ops.begin(), thd.ops.end(), std::back_inserter(t->ops));
    t->pc = t->ops.size();
    
    if (init.type->call(curr_scope(*t), init, false)) {
      start(*t);
    } else {
      ERROR(Snabel, "Failed initializing thread");
    }
    
    return *t;
  }
}
