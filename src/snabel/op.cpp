#include <algorithm>
#include <iostream>

#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/func.hpp"
#include "snabel/list.hpp"
#include "snabel/op.hpp"
#include "snabel/proc.hpp"
#include "snackis/core/defer.hpp"

namespace snabel {
  OpImp::OpImp(OpCode code, const str &name):
    code(code), name(name), pc(-1)
  { }
  
  str OpImp::info() const {
    return "";
  }

  bool OpImp::prepare(Scope &scp) {
    return true;
  }

  bool OpImp::refresh(Scope &scp) {
    return false;
  }

  bool OpImp::compile(const Op &op, Scope &scp, OpSeq &out) {
    return false;
  }
  
  bool OpImp::finalize(const Op &op, Scope &scp, OpSeq &out) {
    return false;
  }

  bool OpImp::run(Scope &) {
    return true;
  }

  Backup::Backup(bool copy):
    OpImp(OP_BACKUP, "backup"), copy(copy)
  { }

  OpImp &Backup::get_imp(Op &op) const {
    return std::get<Backup>(op.data);
  }

  str Backup::info() const { return copy ? "copy" : ""; }

  bool Backup::run(Scope &scp) {
    backup_stack(scp.thread, copy);
    return true;
  }

  Break::Break(int64_t dep):
    OpImp(OP_BREAK, "break"), depth(dep)
  { }

  OpImp &Break::get_imp(Op &op) const {
    return std::get<Break>(op.data);
  }

  bool Break::run(Scope &scp) {
    return _break(scp.thread, depth);
  }
  
  Call::Call(opt<Box> target):
    OpImp(OP_CALL, "call"), target(target)
  { }

  OpImp &Call::get_imp(Op &op) const {
    return std::get<Call>(op.data);
  }

  bool Call::run(Scope &scp) {
    if (target) { return target->type->call(scp, *target, false); }
    auto tgt(try_pop(scp.thread));
    
    if (!tgt) {
      ERROR(Snabel, "Missing call target");
      return false;
    }
    
    bool ok(tgt->type->call(scp, *tgt, false));
    return ok;
  }

  Deref::Deref(const Sym &name):
    OpImp(OP_DEREF, "deref"), name(name), compiled(false)
  { }

  OpImp &Deref::get_imp(Op &op) const {
    return std::get<Deref>(op.data);
  }

  str Deref::info() const { return snabel::name(name); }

  bool Deref::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }
    
    compiled = true;
    auto &exe(scp.exec);
    auto fnd(find_env(scp, name));
    if (!fnd) { return false; }

    if (fnd->type == &exe.func_type) {
      out.emplace_back(Funcall(*get<Func *>(*fnd)));
    } else {
      return false;
    }

    return true;
  }

  bool Deref::run(Scope &scp) {
    auto fnd(find_env(scp, name));
    
    if (!fnd) {
      ERROR(Snabel, fmt("Unknown identifier: %0", snabel::name(name)));
      return false;
    }

    return fnd->type->call(scp, *fnd, false);
  }

  Drop::Drop(size_t count):
    OpImp(OP_DROP, "drop"), count(count)
  { }

  OpImp &Drop::get_imp(Op &op) const {
    return std::get<Drop>(op.data);
  }

  str Drop::info() const {
    return fmt_arg(count);
  }
  
  bool Drop::compile(const Op &op, Scope &scp, OpSeq &out) {
    bool done(false);
    size_t rem(0);

    while (!done && count > 0 && !out.empty()) {
      auto &o(out.back());
      
      switch(o.imp.code) {
      case OP_PUSH: {
	auto &p(get<Push>(o.data));
	while (count > 0 && !p.vals.empty()) {
	  count--;
	  rem++;
	  p.vals.pop_back();
	}

	if (p.vals.empty()) {
	  out.pop_back();
	} else {
	  done = true;
	}
	break;
      }
      case OP_DROP: {
	count += get<Drop>(o.data).count;
	rem++;
	out.pop_back();
	break;
      }
      default:
	done = true;
      }
    }
      
    if (rem) {
      if (count) { out.push_back(op); }
      return true;
    }
    
    return false;
  }
  
  bool Drop::run(Scope &scp) {
    auto &s(curr_stack(scp.thread));
    
    if (s.size() < count) {
      ERROR(Snabel, fmt("Not enough values on stack (%0):\n%1", count, s));
      return false;
    }

    s.erase(std::next(s.begin(), s.size()-count), s.end());
    return true;
  }

  Dup::Dup():
    OpImp(OP_DUP, "dup")
  { }

  OpImp &Dup::get_imp(Op &op) const {
    return std::get<Dup>(op.data);
  }

  bool Dup::run(Scope &scp) {
    auto &s(curr_stack(scp.thread));
    if (s.empty()) {
      ERROR(Snabel, "Invalid dup");
      return false;
    }

    s.push_back(s.back());
    return true;
  }

  Fmt::Fmt(const str &in):
    OpImp(OP_FMT, "fmt"), in(in)
  {
    for (size_t i(0); i < in.size(); i++) {
      if (!i || in[i-1] != '\\') {
	switch(in[i]) {
	case '$': {
	  if (i == in.size()-1 || !isdigit(in[i+1])) {
	    ERROR(Snabel, fmt("Invalid format directive: %0", in.substr(i)));
	    i = in.size();
	    break;
	  }
	  
	  subs.emplace_back(i, in.substr(i, 2));
	  i += 2;
	  break;
	}
	case '@': {
	  auto j(in.find(';', i));
	  if (j == str::npos) { j = in.size(); }  
	  subs.emplace_back(i, in.substr(i, j-i));
	  i = j+1;
	  break;
	}}
      }
    }
  }

  OpImp &Fmt::get_imp(Op &op) const {
    return std::get<Fmt>(op.data);
  }

  bool Fmt::run(Scope &scp) {
    auto &stack(curr_stack(scp.thread));
    str out(in);
    int64_t offs(0);
    
    for (auto &s: subs) {
      switch(s.second.at(0)) {
      case '$': {
	auto i(s.second.at(1) - '0');
	
	if (i >= stack.size()) {
	  ERROR(Snabel, fmt("Format failed: %0\n%1",
			    in.substr(s.first+offs),
			    stack));
	  return false;
	}
	
	auto &v(stack.at(stack.size()-i-1));
	auto vs(v.type->fmt(v));
	out.replace(s.first+offs, s.second.size(), vs);
	offs += vs.size() - s.second.size();
	break;
      }
      case '@': {
	auto v(find_env(scp, s.second));
	
	if (!v) {
	  ERROR(UnknownId, get_sym(scp.exec, s.second));
	  return false;
	}

	auto vs(v->type->fmt(*v));
	out.replace(s.first+offs, s.second.size()+1, vs);
	offs += vs.size() - s.second.size();
	break;
      }
      default:
	ERROR(Snabel, fmt("Invalid format directive: %0", s.second));
	return false;
      }
    }
    
    push(scp.thread, scp.exec.str_type, std::make_shared<str>(out));
    return true;
  }

  For::For(bool push_vals):
    OpImp(OP_FOR, "for"), push_vals(push_vals), compiled(false)
  { }

  OpImp &For::get_imp(Op &op) const {
    return std::get<For>(op.data);
  }

  bool For::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }
    compiled = true;
    auto &exe(scp.exec);
    auto &lbl(add_label(exe, fmt("_for%0", uid(exe))));
    out.emplace_back(Target(lbl));
    out.push_back(op);
    out.emplace_back(Jump(lbl));
    return true;
  }

  bool For::run(Scope &scp) {
    auto &thd(scp.thread);
    IterRef it;
    opt<Box> tgt;
    auto fnd(scp.op_state.find(pc));

    if (fnd == scp.op_state.end()) {    
      tgt = try_pop(thd);

      if (!tgt) {
	ERROR(Snabel, "Missing for target");
	return false;
      }
    
      auto cnd(try_pop(thd));
    
      if (!cnd) {
	ERROR(Snabel, "Missing for condition");
	return false;
      }
        
      if (!cnd->type->iter)     {
	ERROR(Snabel, fmt("Invalid for condition: %0", *cnd));
	return false;
      }

      it = (*cnd->type->iter)(*cnd);
      scp.op_state.emplace(std::piecewise_construct,
			   std::forward_as_tuple(pc),
			   std::forward_as_tuple(State(it, *tgt)));
    } else {
      auto &s(get<State>(fnd->second));
      it = s.iter;
      tgt.emplace(s.target);
    }

    auto nxt(it->next(scp));
    
    if (nxt) {      
      if (tgt->type == &scp.exec.drop_type) { return true; }
      if (push_vals) { push(thd, *nxt); }
      if (tgt->type == &scp.exec.nop_type) { return true; }
      scp.break_pc = thd.pc+2;
      if (!tgt->type->call(scp, *tgt, false)) { return false; }
    } else {
      scp.op_state.erase(pc);
      scp.break_pc = -1;
      thd.pc += 2;
    }
    
    return true;
  }

  For::State::State(const IterRef &itr, const Box &tgt):
    iter(itr), target(tgt)
  { }

  Funcall::Funcall(Func &fn):
    OpImp(OP_FUNCALL, "funcall"), fn(fn), imp(nullptr)
  { }

  OpImp &Funcall::get_imp(Op &op) const {
    return std::get<Funcall>(op.data);
  }

  str Funcall::info() const {
    return snabel::name(fn.name);
  }

  bool Funcall::prepare(Scope &scp) {
    if (scp.safe_level &&
	std::find_if(fn.imps.begin(), fn.imps.end(), [](auto &i) {
	    return i.sec != Func::Unsafe;
	  }) == fn.imps.end()) {
      ERROR(UnsafeCall, fn);
      return false;
    }

    return true;
  }
  
  bool Funcall::run(Scope &scp) {
    auto &thd(scp.thread);
    
    if (imp) {
      auto m(match(*imp, thd, true));

      if (m) {
	(*imp)(scp, *m);
	return true;
      }
    }

    auto m(match(fn, thd));
    
    if (!m) {
      ERROR(Snabel, fmt("Function not applicable: %0\n%1", 
			snabel::name(fn.name), curr_stack(thd)));
      return false;
    }

    imp = m->first;
    if (scp.safe_level && imp->sec >= Func::Unsafe) {
      ERROR(UnsafeCall, fn);
      return false;
    }
     
    (*imp)(scp, m->second);
    return true;
  }
  
  Getenv::Getenv(opt<Sym> id):
    OpImp(OP_GETENV, "getenv"), id(id)
  { }

  OpImp &Getenv::get_imp(Op &op) const {
    return std::get<Getenv>(op.data);
  }

  str Getenv::info() const { return id ? snabel::name(*id) : ""; }

  bool Getenv::run(Scope &scp) {
    auto &exe(scp.exec);
    auto &thd(scp.thread);
    auto id(this->id);
    
    if (!id) {
      auto id_arg(try_pop(thd));
      
      if (!id_arg) {
	ERROR(Snabel, "Missing identifier");
	return false;
      }
      
      if (id_arg->type != &exe.sym_type) {
	ERROR(Snabel, fmt("Invalid identifier: %0", *id_arg));
	return false;
      }
      
      id = get<Sym>(*id_arg);
    }
    
    auto fnd(find_env(scp, *id));

    if (!fnd) {
      ERROR(UnknownId, *id);
      return false;
    }
    
    push(thd, *fnd);
    return true;
  }

  Jump::Jump(const Sym &tag):
    OpImp(OP_JUMP, "jump"), tag(tag), label(nullptr)
  { }

  Jump::Jump(Label &label):
    OpImp(OP_JUMP, "jump"), tag(label.tag), label(&label)
  { }

  OpImp &Jump::get_imp(Op &op) const {
    return std::get<Jump>(op.data);
  }

  str Jump::info() const {
    return fmt("%0:%1", snabel::name(tag), label ? to_str(label->pc) : "?");
  }

  bool Jump::refresh(Scope &scp) {
    if (!label) { label = find_label(scp.exec, tag); }
    return false;
  }

  bool Jump::run(Scope &scp) {
    if (!label) {
      ERROR(Snabel, fmt("Missing label: %0", snabel::name(tag)));
      return false;
    }

    jump(scp, *label);
    return true;
  }
  
  Lambda::Lambda():
    OpImp(OP_LAMBDA, "lambda"),
    enter_label(nullptr),
    skip_label(nullptr),
    safe_level(-1),
    compiled(false)
  { }

  OpImp &Lambda::get_imp(Op &op) const {
    return std::get<Lambda>(op.data);
  }

  bool Lambda::prepare(Scope &scp) {
    auto &exe(scp.exec);
    tag = uid(exe);
    enter_label = &add_label(exe, fmt("_enter%0", tag));
    skip_label = &add_label(exe, fmt("_skip%0", tag));
    return true;
  }

  bool Lambda::refresh(Scope &scp) {
    auto &exe(scp.exec);
    exe.lambdas.push_back(this);
    begin_scope(scp.thread, false);
    return false;
  }

  bool Lambda::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }
    compiled = true;
    out.emplace_back(Jump(*skip_label));
    out.emplace_back(Target(*enter_label));
    out.push_back(op);
    return true;
  }

  bool Lambda::run(Scope &scp) {
    auto &thd(scp.thread);
    Scope &new_scp(begin_scope(thd, !(scp.coro && scp.coro->proc)));
    new_scp.target = enter_label;
    new_scp.recall_pc = thd.pc+1;
    
    if (scp.coro) {
      auto &cor(scp.coro);
      thd.pc = cor->pc;
      new_scp.safe_level = cor->safe_level;
      if (cor->proc) { new_scp.push_result = false; }
      std::copy(cor->stacks.begin(), cor->stacks.end(),
		std::back_inserter(thd.stacks));
      std::copy(cor->op_state.begin(), cor->op_state.end(),
		std::inserter(new_scp.op_state, new_scp.op_state.end()));
      new_scp.env.swap(cor->env);
    } else {
      new_scp.safe_level = safe_level;
    }

    return true;
  }

  Loop::Loop():
    OpImp(OP_LOOP, "loop"), compiled(false)
  { }

  OpImp &Loop::get_imp(Op &op) const {
    return std::get<Loop>(op.data);
  }

  bool Loop::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }
    compiled = true;
    auto &exe(scp.exec);
    auto &lbl(add_label(exe, fmt("_loop%0", uid(exe))));
    out.emplace_back(Target(lbl));
    out.push_back(op);
    out.emplace_back(Jump(lbl));
    return true;
  }

  bool Loop::run(Scope &scp) {
    auto &thd(scp.thread);

    opt<Box> tgt;
    auto fnd(scp.op_state.find(pc));

    if (fnd == scp.op_state.end()) {
      tgt = try_pop(thd);
    
      if (!tgt) {
	ERROR(Snabel, "Missing while target");
	return false;
      }
      
      scp.op_state.emplace(std::piecewise_construct,
			   std::forward_as_tuple(pc),
			   std::forward_as_tuple(State(*tgt)));
    } else {
      auto &s(get<State>(fnd->second));
      tgt.emplace(s.target);
    }
    
    if (tgt->type == &scp.exec.nop_type) { return true; }
    scp.break_pc = scp.thread.pc+2;
    tgt->type->call(scp, *tgt, false);
    return true;
  }

  Loop::State::State(const Box &tgt):
    target(tgt)
  { }
  
  Putenv::Putenv(const Sym &key):
    OpImp(OP_PUTENV, "putenv"), key(key)
  { }

  OpImp &Putenv::get_imp(Op &op) const {
    return std::get<Putenv>(op.data);
  }

  str Putenv::info() const { return snabel::name(key); }

  bool Putenv::prepare(Scope &scp) {
    auto fnd(find_env(scp, key));
    if (fnd) { ERROR(Snabel, fmt("Rebinding symbol: %0", snabel::name(key))); }
    return true;
  }
  
  bool Putenv::run(Scope &scp) {
    auto val(try_pop(scp.thread));

    if (!val) {
      ERROR(Snabel, fmt("Missing symbol value: %0", snabel::name(key)));
      return false;
    }

    put_env(scp, key, *val);
    return true;
  }

  Push::Push(const Box &val):
    OpImp(OP_PUSH, "push"), vals({val})
  { }

  Push::Push(const Stack &vals):
    OpImp(OP_PUSH, "push"), vals(vals)
  { }
  
  OpImp &Push::get_imp(Op &op) const {
    return std::get<Push>(op.data);
  }

  str Push::info() const { return fmt_arg(vals); }

  bool Push::compile(const Op &op, Scope &scp, OpSeq &out) {
    int popped(0);
    
    while (!out.empty()) {
      auto op(out.back());
      if (op.imp.code != OP_PUSH) { break; }
      auto vs(get<Push>(op.data).vals);
      std::copy(vs.rbegin(), vs.rend(), std::front_inserter(vals));
      out.pop_back();
      popped++;
    }

    if (!popped) { return false; }    
    out.push_back(op);
    return true;
  }
  
  bool Push::run(Scope &scp) {
    push(scp.thread, vals);
    return true;
  }

  Recall::Recall(int64_t dep):
    OpImp(OP_RECALL, "recall"), label(nullptr), depth(dep)
  { }

  OpImp &Recall::get_imp(Op &op) const {
    return std::get<Recall>(op.data);
  }
  
  bool Recall::run(Scope &scp) {
    return recall(scp, depth);
  }
  
  Reset::Reset():
    OpImp(OP_RESET, "reset")
  { }

  OpImp &Reset::get_imp(Op &op) const {
    return std::get<Reset>(op.data);
  }

  bool Reset::finalize(const Op &op, Scope &scp, OpSeq &out) {
    for (auto i(out.rbegin()); i != out.rend(); i++) {
      if (i->imp.code == OP_BACKUP) {
	get<Backup>(i->data).copy = false;
      } else {
	break;
      }
    }

    return false;
  }
  
  bool Reset::run(Scope &scp) {
    curr_stack(scp.thread).clear();
    return true;
  }

  Restore::Restore():
    OpImp(OP_RESTORE, "restore")
  { }

  OpImp &Restore::get_imp(Op &op) const {
    return std::get<Restore>(op.data);
  }

  bool Restore::run(Scope &scp) {
    restore_stack(scp);
    return true;
  }

  Return::Return(int64_t dep):
    OpImp(OP_RETURN, "return"), target(nullptr), depth(dep)
  { }

  OpImp &Return::get_imp(Op &op) const {
    return std::get<Return>(op.data);
  }

  bool Return::refresh(Scope &scp) {
    auto &exe(scp.exec);
    
    if (exe.lambdas.empty()) {
      ERROR(Snabel, "Missing lambda");
      return false;
    }
    
    auto l(exe.lambdas.back());
    target = l->enter_label;
    return false;
  }

  bool Return::run(Scope &scp) {
    return _return(scp, depth);
  }
  
  Safe::Safe():
    OpImp(OP_SAFE, "safe")
  { }

  OpImp &Safe::get_imp(Op &op) const {
    return std::get<Safe>(op.data);
  }

  bool Safe::refresh(Scope &scp) {
    scp.safe_level++;
    return false;
  }

  bool Safe::finalize(const Op &op, Scope &scp, OpSeq & out) {
    return false;
  }

  Stash::Stash():
    OpImp(OP_STASH, "stash")
  { }

  OpImp &Stash::get_imp(Op &op) const {
    return std::get<Stash>(op.data);
  }

  bool Stash::run(Scope &scp) {
    auto &exe(scp.exec);
    auto &thd(scp.thread);
    std::shared_ptr<List> lst(new List());
    lst->swap(curr_stack(thd));

    Type *elt(lst->empty() ? &exe.any_type : lst->at(0).type);

    if (!lst->empty()) {
      for (auto i(std::next(lst->begin())); i != lst->end() && elt; i++) {
	elt = get_super(exe, *elt, *i->type);
      }
    }
    
    push(thd, Box(get_list_type(exe, elt ? *elt : exe.any_type), lst));
    return true;
  }

  Swap::Swap(size_t pos):
    OpImp(OP_SWAP, "swap"), pos(pos)
  { }

  OpImp &Swap::get_imp(Op &op) const {
    return std::get<Swap>(op.data);
  }

  str Swap::info() const { return fmt_arg(pos); }

  bool Swap::run(Scope &scp) {
    auto &s(curr_stack(scp.thread));
    if (s.size() < pos+1) {
      ERROR(Snabel, fmt("Invalid swap: %0\n%1", pos, s));
      return false;
    }

    auto i(std::next(s.begin(), s.size()-pos-1));
    std::rotate(i, std::next(i), s.end());
    return true;
  }

  Target::Target(Label &label):
    OpImp(OP_TARGET, "target"), label(label)
  { }

  OpImp &Target::get_imp(Op &op) const {
    return std::get<Target>(op.data);
  }

  str Target::info() const {
    return fmt("%0:%1", snabel::name(label.tag), label.pc);
  }

  bool Target::finalize(const Op &op, Scope &scp, OpSeq & out) {
    label.pc = scp.thread.pc;    
    return true;
  }

  Unlambda::Unlambda():
    OpImp(OP_UNLAMBDA, "unlambda"),
    enter_label(nullptr), skip_label(nullptr),
    compiled(false)
  { }

  OpImp &Unlambda::get_imp(Op &op) const {
    return std::get<Unlambda>(op.data);
  }

  bool Unlambda::refresh(Scope &scp) {
    Exec &exe(scp.exec);
    
    if (exe.lambdas.empty()) {
      ERROR(Snabel, "Missing lambda");
      return false;
    }

    auto &l(*exe.lambdas.back());
    enter_label = l.enter_label;
    skip_label = l.skip_label;
    l.safe_level = scp.safe_level;
    exe.lambdas.pop_back();
    end_scope(scp.thread);
    return false;
  }
  
  bool Unlambda::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }  
    
    compiled = true;
    out.push_back(op);
    out.emplace_back(Target(*skip_label));
    out.emplace_back(Push(Box(scp.exec.lambda_type, enter_label)));
    return true;
  }

  bool Unlambda::run(Scope &scp) {
    return _return(scp, 1);
  }
  
  Yield::Yield(int64_t depth):
    OpImp(OP_YIELD, "yield"), depth(depth)
  { }

  OpImp &Yield::get_imp(Op &op) const {
    return std::get<Yield>(op.data);
  }

  str Yield::info() const { return fmt_arg(depth); }

  bool Yield::run(Scope &scp) {
    return yield(scp, depth);
  }

  Op::Op(const Op &src):
    data(src.data), imp(src.imp.get_imp(*this)), prepared(src.prepared)
  { }

  bool prepare(Op &op, Scope &scp) {
    op.prepared = true;
    return op.imp.prepare(scp);
  }

  bool refresh(Op &op, Scope &scp) {
    return op.imp.refresh(scp);
  }

  bool compile(Op &op, Scope &scp, OpSeq &out) {
    if (op.imp.compile(op, scp, out)) { return true; }
    out.push_back(op);
    return false;
  }

  bool finalize(Op &op, Scope &scp, OpSeq &out) {
    if (op.imp.finalize(op, scp, out)) { return true; }
    op.imp.pc = scp.thread.pc;
    out.push_back(op);
    return false;
  }
  
  bool run(Op &op, Scope &scp) {
    return op.imp.run(scp);
  }
}
