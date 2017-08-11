#include <algorithm>
#include <iostream>

#include "snabel/box.hpp"
#include "snabel/coro.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/func.hpp"
#include "snabel/op.hpp"
#include "snackis/core/defer.hpp"

namespace snabel {
  OpImp::OpImp(OpCode code, const str &name):
    code(code), name(name)
  { }
  
  str OpImp::info() const {
    return "";
  }

  void OpImp::prepare(Scope &scp)
  { }

  void OpImp::refresh(Scope &scp)
  { }

  bool OpImp::trace(Scope &scp) {
    return true;
  }

  bool OpImp::compile(const Op &op, Scope &scp, OpSeq &out) {
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

  bool Backup::trace(Scope &scp) {
    return run(scp);
  }
  
  bool Backup::run(Scope &scp) {
    backup_stack(scp.coro, copy);
    return true;
  }
  
  Branch::Branch():
    OpImp(OP_BRANCH, "branch"), label(nullptr)
  { }

  OpImp &Branch::get_imp(Op &op) const {
    return std::get<Branch>(op.data);
  }

  str Branch::info() const {
      return label ? label->tag : "";    
  }

  bool Branch::trace(Scope &scp) {
    return run(scp);
  }

  /*
  out.emplace_back(Trunc(curr_stack(cor).size()));
  */

  bool Branch::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (!cond) { return false; }
    
    if (*cond) {
      out.emplace_back(Swap());
      out.emplace_back(Drop(1));
      out.emplace_back(Call());
    } else {
      out.emplace_back(Drop(2));	
    }
      
    return true;
  }
  
  bool Branch::run(Scope &scp) {
    auto &cor(scp.coro);
    auto &exe(cor.exec);
    
    auto lbl(peek(cor));
    if (!lbl) { return false; }      
    pop(cor);

    if (!label) {
      const str l(get<str>(*lbl));
      auto fnd(exe.labels.find(l));
      if (fnd == exe.labels.end()) {
	ERROR(Snabel, fmt("Missing branch label: %0", l));
	return false;
      }
      
      label = &fnd->second;
    }

    auto cnd(peek(cor));
    
    if (cnd) {
      pop(cor);
      if (cnd->type != &cor.exec.bool_type) {
	ERROR(Snabel, fmt("Invalid branch condition: %0", *cnd));
	return false;
      }
      
      cond.emplace(get<bool>(*cnd));
    } else {
      ERROR(Snabel, fmt("Missing branch condition: %0", curr_stack(cor)));
      return false;
    }
    
    if (!cond) { return false; }    
    if (*cond) { call(cor, *label); }
    return true;    
  }
  
  Call::Call():
    OpImp(OP_CALL, "call"), label(nullptr)
  { }

  Call::Call(Label &label):
    OpImp(OP_CALL, "call"), label(&label)
  { }

  OpImp &Call::get_imp(Op &op) const {
    return std::get<Call>(op.data);
  }

  str Call::info() const {
      return label ? label->tag : "";    
  }

  bool Call::trace(Scope &scp) {
    auto &cor(scp.coro);
    auto &exe(cor.exec);      
      
    if (!label) {
      auto fn(peek(cor));

      if (fn && fn->type == &exe.lambda_type) {
	auto fnd(exe.labels.find(get<str>(*fn)));
	if (fnd != exe.labels.end()) { label = &fnd->second; }
      }
    }

    if (!label) { return false; }
    
    return run(scp);
  }
  
  bool Call::run(Scope &scp) {
    auto &cor(scp.coro);
    auto &exe(cor.exec);
    auto fn(pop(cor));
      
    if (!label) {
      str n(get<str>(fn));
      auto fnd(exe.labels.find(n));

      if (fnd == exe.labels.end()) {
	ERROR(Snabel, fmt("Missing call target: %0", n));
	return false;
      } else {
	label = &fnd->second;
      }      
    }

    call(cor, *label);
    return true;
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
  
  bool Drop::trace(Scope &scp) {
    return run(scp);
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
    auto &s(curr_stack(scp.coro));
    if (s.size() < count) {
      ERROR(Snabel, fmt("Not enough values on stack (%0):\n%1", count, s));
      return false;
    }
    
    for (size_t i(0); i < count; i++) { pop(scp.coro); }
    return true;
  }

  Exit::Exit():
    OpImp(OP_EXIT, "exit"), label(nullptr)
  { }

  OpImp &Exit::get_imp(Op &op) const {
    return std::get<Exit>(op.data);
  }

  bool Exit::trace(Scope &scp) {
    Coro &cor(scp.coro);
    Exec &exe(cor.exec);

    if (!label) {
      if (exe.lambdas.empty()) {
	ERROR(Snabel, "Missing lambda");
	return false;
      }

      str tag(exe.lambdas.back());
      auto lbl(find_label(exe, fmt("_exit%0", tag)));

      if (!lbl) {
	ERROR(Snabel, "Missing exit label");
	return false;
      }

      label = lbl;
    }

    return true;
  }
  
  bool Exit::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (!label) { return false; }
    out.emplace_back(Jump(*label));
    return true;
  }

  bool Exit::run(Scope &scp) {
    return false;
  }

  Funcall::Funcall(Func &fn):
    OpImp(OP_FUNCALL, "funcall"), fn(fn), imp(nullptr)
  { }

  OpImp &Funcall::get_imp(Op &op) const {
    return std::get<Funcall>(op.data);
  }

  str Funcall::info() const {
    return fn.name;
  }

  bool Funcall::trace(Scope &scp) {
    Coro &cor(scp.coro);      
    if (!imp) { imp = match(fn, cor); }
    if (!imp) { return false; }
    auto args(pop_args(*imp, cor));
    	
    if (result) {
      push(cor, *result);
    } else {
      if (args.size() < imp->args.size()) { return false; }

      if (imp->pure &&
	  !imp->args.empty() &&
	  std::find_if(args.begin(), args.end(),
		       [](auto &a){ return undef(a); }) == args.end()) {
	(*imp)(cor, args);
	auto &s(curr_stack(cor));
	result.emplace(Stack(std::next(s.begin(), s.size()-imp->results.size()),
			     s.end()));
      } else {
	for (auto &res: imp->results) {
	  auto rt(get_type(*imp, res, args));

	  if (!rt) {
	    ERROR(Snabel, "Missing function result type");
	    return false;
	  }

	  push(cor, Box(*rt, undef));
	}
      }
    }
    
    return true;
  }
  
  bool Funcall::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (result) {
      out.emplace_back(Drop(imp->args.size()));	
      out.emplace_back(Push(*result));
      return true;
    }

    return false;
  }
  
  bool Funcall::run(Scope &scp) {
    Coro &cor(scp.coro);
    if (!imp) { imp = match(fn, cor); }
    
    if (!imp) {
      ERROR(Snabel, fmt("Function not applicable: %0\n%1", 
			fn.name, curr_stack(scp.coro)));
      return false;
    }

    (*imp)(cor);
    return true;
  }

  Get::Get(const str &name):
    OpImp(OP_GET, "get"), name(name)
  { }

  OpImp &Get::get_imp(Op &op) const {
    return std::get<Get>(op.data);
  }

  str Get::info() const { return name; }

  bool Get::trace(Scope &scp) {
    return run(scp);
  }
  
  bool Get::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (!val) { return false; }
    
    if (val->type == &scp.coro.exec.func_type &&
	name.front() != '$') {
      out.emplace_back(Funcall(*get<Func *>(*val)));
    } else {
      out.emplace_back(Push(*val));
    }
      
    return true;
  }
  
  bool Get::run(Scope &scp) {
    if (!val) {
      auto fnd(find_env(scp, name));

      if (!fnd) {
	ERROR(Snabel, fmt("Unknown identifier: %0", name));
	return false;
      }

      val.emplace(*fnd);
    }

    if (val->type == &scp.coro.exec.func_type &&
	name.front() != '$') {
      ERROR(Snabel, fmt("Function not found: %0", name));
      return false;
    }
    
    push(scp.coro, *val);
    return true;
  }
  
  Group::Group(bool copy):
    OpImp(OP_GROUP, "group"), copy(copy)
  { }

  OpImp &Group::get_imp(Op &op) const {
    return std::get<Group>(op.data);
  }

  str Group::info() const { return copy ? "copy" : ""; }

  bool Group::trace(Scope &scp) {
    return run(scp);
  }
  
  bool Group::run(Scope &scp) {
    begin_scope(scp.coro, copy);
    return true;
  }

  Jump::Jump(const str &tag):
    OpImp(OP_JUMP, "jump"), tag(tag), label(nullptr)
  { }

  Jump::Jump(Label &label):
    OpImp(OP_JUMP, "jump"), tag(label.tag), label(&label)
  { }

  OpImp &Jump::get_imp(Op &op) const {
    return std::get<Jump>(op.data);
  }

  str Jump::info() const {
    return fmt("%0 (%1)", tag, label ? to_str(label->pc) : "?");
  }

  bool Jump::trace(Scope &scp) {
    auto &cor(scp.coro);
    auto &exe(cor.exec);
    
    auto fnd(exe.labels.find(tag));
    if (fnd == exe.labels.end()) { return false; }
    
    label = &fnd->second;
    return run(scp);
  }

  bool Jump::run(Scope &scp) {
    if (!label) {
      ERROR(Snabel, fmt("Missing label: %0", tag));
      return false;
    }
      
    jump(scp.coro, *label);
    return true;
  }
  
  Lambda::Lambda():
    OpImp(OP_LAMBDA, "lambda"), compiled(false)
  { }

  OpImp &Lambda::get_imp(Op &op) const {
    return std::get<Lambda>(op.data);
  }

  void Lambda::prepare(Scope &scp) {
    tag = fmt_arg(gensym(scp.coro.exec));
  }
  
  bool Lambda::trace(Scope &scp) {
    return run(scp);
  }
  
  bool Lambda::compile(const Op &op, Scope &scp, OpSeq &out) {
      if (compiled) { return false; }
      if (!run(scp)) { return false; }

      if (tag.empty()) {
	ERROR(Snabel, "Empty lambda tag");
	return false;
      }

      compiled = true;
      out.emplace_back(Jump(fmt("_skip%0", tag)));
      out.emplace_back(Target(fmt("_enter%0", tag)));
      out.emplace_back(Group(true));
      out.push_back(op);
      return true;
  }

  bool Lambda::run(Scope &scp) {
    scp.coro.exec.lambdas.push_back(tag);
    return true;
  }
  
  Let::Let(const str &name):
    OpImp(OP_LET, "let"), name(name)
  { }

  OpImp &Let::get_imp(Op &op) const {
    return std::get<Let>(op.data);
  }

  str Let::info() const { return name; }

  void Let::prepare(Scope &scp) {
    auto fnd(find_env(scp, name));
      
    if (fnd) {
      ERROR(Snabel, fmt("Duplicate binding: %0", name));
    }
  }
  
  bool Let::trace(Scope &scp) {
    auto &s(curr_stack(scp.coro));
    if (s.empty()) { return false; }
    
    if (!val) {
      val.emplace(s.back());
      put_env(scp, name, *val);
    }
    
    s.pop_back();
    return true;
  }
  
  bool Let::run(Scope &scp) {
    auto &s(curr_stack(scp.coro));

    if (s.empty()) {
      ERROR(Snabel, fmt("Missing bound val: %0", name));
      return false;
    }

    val.emplace(s.back());
    s.pop_back();
    put_env(scp, name, *val);
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

  bool Push::trace(Scope &scp) {
    return run(scp);
  }
  
  bool Push::run(Scope &scp) {
    push(scp.coro, vals);
    return true;
  }

  Reset::Reset():
    OpImp(OP_RESET, "reset")
  { }

  OpImp &Reset::get_imp(Op &op) const {
    return std::get<Reset>(op.data);
  }

  bool Reset::trace(Scope &scp) {
    return run(scp);
  }

  bool Reset::run(Scope &scp) {
    curr_stack(scp.coro).clear();
    return true;
  }

  Restore::Restore():
    OpImp(OP_RESTORE, "restore")
  { }

  OpImp &Restore::get_imp(Op &op) const {
    return std::get<Restore>(op.data);
  }

  bool Restore::trace(Scope &scp) {
    return run(scp);
  }

  bool Restore::run(Scope &scp) {
    restore_stack(scp.coro);
    return true;
  }

  Return::Return():
    OpImp(OP_RETURN, "return")
  { }

  OpImp &Return::get_imp(Op &op) const {
    return std::get<Return>(op.data);
  }

  bool Return::trace(Scope &scp) {
    if (scp.coro.returns.empty()) { return false; }
    return run(scp);
  }

  bool Return::run(Scope &scp) {
    Coro &cor(scp.coro);

    if (cor.returns.empty()) {
      ERROR(Snabel, "Missing return pc");
      return false;
    }

    cor.pc = cor.returns.back();
    cor.returns.pop_back();
    return true;
  }
  
  Stash::Stash():
    OpImp(OP_STASH, "stash")
  { }

  OpImp &Stash::get_imp(Op &op) const {
    return std::get<Stash>(op.data);
  }

  bool Stash::trace(Scope &scp) {
    return run(scp);
  }

  bool Stash::run(Scope &scp) {
    Coro &cor(scp.coro);
    Exec &exe(cor.exec);
    std::shared_ptr<List> lst(new List());
    lst->elems.swap(curr_stack(cor));

    Type *elt(lst->elems.empty() ? &exe.any_type : lst->elems[0].type);  
    for (auto i(std::next(lst->elems.begin())); i != lst->elems.end() && elt; i++) {
      elt = get_super(*elt, *i->type);
    }

    push(cor, Box(get_list_type(exe, elt ? *elt : exe.undef_type), lst));
    return true;
  }

  Swap::Swap():
    OpImp(OP_SWAP, "swap")
  { }

  OpImp &Swap::get_imp(Op &op) const {
    return std::get<Swap>(op.data);
  }

  bool Swap::trace(Scope &scp) {
    return run(scp);
  }
    
  bool Swap::run(Scope &scp) {
    auto &s(curr_stack(scp.coro));
    if (s.size() < 2) {
      ERROR(Snabel, fmt("Invalid swap:\n%0", s));
      return false;
    }
    
    auto x(s.back());
    s.pop_back();
    auto y(s.back());
    s.pop_back();
    s.push_back(x);
    s.push_back(y);
    return true;
  }

  Target::Target(const str &tag):
    OpImp(OP_TARGET, "target"), tag(tag), label(nullptr)
  { }

  OpImp &Target::get_imp(Op &op) const {
    return std::get<Target>(op.data);
  }

  str Target::info() const { return tag; }

  void Target::prepare(Scope &scp) {
    auto &cor(scp.coro);
    auto &exe(cor.exec);
    auto fnd(exe.labels.find(tag));
    pc = cor.pc;
    depth = cor.scopes.size();

    if (fnd == exe.labels.end()) {
      label = &exe.labels
	.emplace(std::piecewise_construct,
		 std::forward_as_tuple(tag),
		 std::forward_as_tuple(tag, cor.pc, depth))
	.first->second;
    } else {
      ERROR(Snabel, fmt("Duplicate label: %0", tag));
    }
  }

  void Target::refresh(Scope &scp) {
    if (label) {
      label->pc = scp.coro.pc;
    } else {
      ERROR(Snabel, "Missing target label");
    }
  }
  
  Ungroup::Ungroup():
    OpImp(OP_UNGROUP, "ungroup")
  { }

  OpImp &Ungroup::get_imp(Op &op) const {
    return std::get<Ungroup>(op.data);
  }

  bool Ungroup::trace(Scope &scp) {
    return run(scp);
  }
  
  bool Ungroup::run(Scope &scp) {
    if (scp.coro.scopes.size() < 2) { return false; }
    end_scope(scp.coro);
    return true;
  }

  Unlambda::Unlambda():
    OpImp(OP_UNLAMBDA, "unlambda"), compiled(false)
  { }

  OpImp &Unlambda::get_imp(Op &op) const {
    return std::get<Unlambda>(op.data);
  }

  bool Unlambda::trace(Scope &scp) {
      return run(scp);
  }

  bool Unlambda::compile(const Op &op, Scope &scp, OpSeq &out) {
      Coro &cor(scp.coro);
      if (compiled) { return false; }  
      if (!run(scp)) { return false; }

      compiled = true;
      out.emplace_back(Target(fmt("_exit%0", tag)));
      out.push_back(op);
      out.emplace_back(Ungroup());
      out.emplace_back(Return());
      out.emplace_back(Target(fmt("_skip%0", tag)));
      out.emplace_back(Push(Box(cor.exec.lambda_type, fmt("_enter%0", tag))));
      return true;
  }

  bool Unlambda::run(Scope &scp) {
    Exec &exe(scp.coro.exec);
    
    if (exe.lambdas.empty()) {
      ERROR(Snabel, "Missing lambda");
      return false;
    }

    if (tag.empty()) {
      tag = exe.lambdas.back();
    } else if (exe.lambdas.back() != tag) {
      ERROR(Snabel, "Lambda tag changed");
      return false;
    }
    
    exe.lambdas.pop_back();
    return true;
  }

  Op::Op(const Op &src):
    data(src.data), imp(src.imp.get_imp(*this)), prepared(src.prepared)
  { }

  void prepare(Op &op, Scope &scp) {
    op.imp.prepare(scp);
    op.prepared = true;
  }

  void refresh(Op &op, Scope &scp) {
    op.imp.refresh(scp);
  }

  bool trace(Op &op, Scope &scp) {
    return op.imp.trace(scp);
  }

  bool compile(Op &op, Scope &scp, OpSeq &out) {
    if (op.imp.compile(op, scp, out)) { return true; }
    out.push_back(op);
    return false;
  }

  bool run(Op &op, Scope &scp) {
    return op.imp.run(scp);
  }
}
