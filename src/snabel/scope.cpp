#include <iostream>
#include "snabel/scope.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snackis/core/error.hpp"

namespace snabel {  
  Scope::Scope(Thread &thd):
    thread(thd),
    exec(thread.exec),
    target(nullptr),
    coro(nullptr),
    stack_depth(thread.stacks.size()),
    return_pc(-1),
    recall_pc(-1),
    break_pc(-1),
    push_result(true)
  { }

  Scope::Scope(const Scope &src):
    thread(src.thread),
    exec(src.exec),
    target(nullptr),
    coro(nullptr),
    stack_depth(thread.stacks.size()),
    return_pc(-1),
    recall_pc(-1),
    break_pc(-1),
    push_result(true)
  {}
  
  Scope::~Scope() {
    reset_stack(*this);
    rollback_env(*this);
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
    auto &env(scp.thread.env);
    auto fnd(env.find(key));
    if (fnd == env.end()) { return nullptr; }
    return &fnd->second;
  }

  Box *find_env(Scope &scp, const str &key) {
    return find_env(scp, get_sym(scp.exec, key));
  }

  void put_env(Scope &scp, const Sym &key, const Box &val) {
    auto &env(scp.thread.env);
    auto fnd(env.find(key));

    if (fnd == env.end()) {
      env.emplace(std::piecewise_construct,
		  std::forward_as_tuple(key),
		  std::forward_as_tuple(val));
    } else {
      ERROR(Snabel, fmt("Rebinding name: %0", name(key)));
      fnd->second = val;
    }

    if (&scp != &scp.thread.main_scope) { scp.env_keys.insert(key); }
  }

  void put_env(Scope &scp, const str &key, const Box &val) {
    put_env(scp, get_sym(scp.exec, key), val);
  }
  
  bool rem_env(Scope &scp, const Sym &key) {
    if (&scp != &scp.thread.main_scope) {
      auto fnd(scp.env_keys.find(key));
      if (fnd == scp.env_keys.end()) { return false; }
      scp.env_keys.erase(fnd);
    }
    
    scp.thread.env.erase(key);
    return true;
  }

  bool rem_env(Scope &scp, const str &key) {
    return rem_env(scp, get_sym(scp.exec, key));
  }

  void rollback_env(Scope &scp) {
    for (auto &k: scp.env_keys) { scp.thread.env.erase(k); }
    scp.env_keys.clear();
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
      auto &s(thd.scopes.back());
      depth--;

      if (!depth) {
	if (s.recalls.empty()) {
	  auto &ps(*std::next(thd.scopes.rbegin()));	  
	  
	  if (ps.return_pc == -1) {
	    ERROR(Snabel, "Missing return pc");
	    return false;
	  }
	  
	  end_scope(thd);
	  thd.pc = ps.return_pc;
	  ps.return_pc = -1;
	  if (ps.coro) { reset(*ps.coro); }
	  ps.coro.reset();
	} else {
	  recall_return(s);
	}

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
	auto &frm(s.recalls.emplace_back());
	refresh(frm, s);
	reset_stack(thd, s.stack_depth, true);
	thd.pc = s.recall_pc;
	break;
      }

      end_scope(thd);
    }

    return true;
  }

  void recall_return(Scope &scp) {
    auto &thd(scp.thread);
    auto &frm(scp.recalls.back());
    thd.pc = frm.pc;
    opt<Box> last;
    if (!thd.stacks.back().empty()) { last = pop(thd); }    
    std::copy(frm.stacks.begin(), frm.stacks.end(),
	      std::back_inserter(thd.stacks));
    if (last) { push(thd, *last); }
    scp.recalls.pop_back();
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
    
    auto &s(curr_stack(thd));
    std::copy(s.begin(), s.end(), std::back_inserter(curr_stack(*t)));

    auto &e(thd.env);
    auto &te(t->env);
    std::copy(e.begin(), e.end(), std::inserter(te, te.end()));

    std::copy(thd.ops.begin(), thd.ops.end(), std::back_inserter(t->ops));
    t->pc = t->ops.size();
    
    if ((*init.type->call)(curr_scope(*t), init, false)) {
      start(*t);
    } else {
      ERROR(Snabel, "Failed initializing thread");
    }
    
    return *t;
  }
}
