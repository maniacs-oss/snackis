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

  bool OpImp::prepare(Scope &scp) {
    return true;
  }

  bool OpImp::refresh(Scope &scp) {
    return true;
  }

  bool OpImp::compile(const Op &op, Scope &scp, OpSeq &out) {
    return false;
  }
  
  bool OpImp::run(Scope &) {
    return true;
  }

  Backup::Backup():
    OpImp(OP_BACKUP, "backup")
  { }

  OpImp &Backup::get_imp(Op &op) const {
    return std::get<Backup>(op.data);
  }

  bool Backup::run(Scope &scp) {
    backup_stack(scp.coro);
    return true;
  }
  
  Branch::Branch(opt<Box> target):
    OpImp(OP_BRANCH, "branch"), target(target)
  { }

  OpImp &Branch::get_imp(Op &op) const {
    return std::get<Branch>(op.data);
  }

  bool Branch::prepare(Scope &scp) {
    if (target && !target->type->call) {
      ERROR(Snabel, fmt("Invalid branch target: %0", *target));
      return false;
    }

    return true;
  }
  
  bool Branch::run(Scope &scp) {
    auto &cor(scp.coro);
    auto tgt(target);

    if (!tgt) {
      tgt = try_pop(cor);
      
      if (!tgt) {
	ERROR(Snabel, "Missing branch target");
	return false;
      }
      
      if (!tgt->type->call) {
	ERROR(Snabel, fmt("Invalid branch target: %0", *tgt));
	return false;
      }
    }

    auto cnd(try_pop(cor));

    if (!cnd) {
      ERROR(Snabel, "Missing branch condition");
      return false;
    }

    if (cnd->type != &cor.exec.bool_type) {
      ERROR(Snabel, fmt("Invalid branch condition: %0", *cnd));
      return false;
    }

    if(get<bool>(*cnd)) { return (*tgt->type->call)(scp, *tgt); }
    return true;
  }
  
  Call::Call(opt<Box> target):
    OpImp(OP_CALL, "call"), target(target)
  { }

  OpImp &Call::get_imp(Op &op) const {
    return std::get<Call>(op.data);
  }

  bool Call::run(Scope &scp) {
    if (target) { return (*target->type->call)(scp, *target); }

    auto &cor(scp.coro);
    auto tgt(peek(cor));
    
    if (!tgt) {
      ERROR(Snabel, "Missing call target");
      return false;
    }
    
    pop(cor);
    
    if (!tgt->type->call) {
      ERROR(Snabel, fmt("Invalid call target: %0", *tgt));
      return false;
    }
    
    return (*tgt->type->call)(scp, *tgt);
  }

  Deref::Deref(const str &name):
    OpImp(OP_DEREF, "deref"), name(name)
  { }

  OpImp &Deref::get_imp(Op &op) const {
    return std::get<Deref>(op.data);
  }

  str Deref::info() const { return name; }

  bool Deref::compile(const Op &op, Scope &scp, OpSeq &out) {
    auto &exe(scp.exec);
    auto fnd(find_env(scp, name));
    if (!fnd) { return false; }

    if (fnd->type == &exe.func_type && name.front() != '$') {
      out.emplace_back(Funcall(*get<Func *>(*fnd)));
    } else if (fnd->type == &exe.lambda_type && name.front() != '$') {
      out.emplace_back(Call(*fnd));
    } else if (fnd->type == &exe.label_type && name.front() != '$') {
      out.emplace_back(Jump(*get<Label *>(*fnd)));
    } else {
      out.emplace_back(Push(*fnd));
    }

    return true;
  }

  bool Deref::run(Scope &scp) {
    auto &cor(scp.coro);
    auto &exe(scp.exec);
    auto fnd(find_env(scp, name));
    
    if (!fnd) {
      ERROR(Snabel, fmt("Unknown identifier: %0", name));
      return false;
    }

    if (isa(*fnd, exe.callable_type) && name.front() != '$') {
      return (*fnd->type->call)(scp, *fnd);
    }
    
    push(cor, *fnd);
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

  Dup::Dup():
    OpImp(OP_DUP, "dup")
  { }

  OpImp &Dup::get_imp(Op &op) const {
    return std::get<Dup>(op.data);
  }

  bool Dup::run(Scope &scp) {
    auto &s(curr_stack(scp.coro));
    if (s.empty()) {
      ERROR(Snabel, "Invalid dup");
      return false;
    }

    s.push_back(s.back());
    return true;
  }

  For::For():
    OpImp(OP_FOR, "for"), compiled(false)
  { }

  OpImp &For::get_imp(Op &op) const {
    return std::get<For>(op.data);
  }

  bool For::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }
    compiled = true;
    auto &lbl(add_label(scp.exec, fmt("_for%0", gensym(scp.exec))));
    out.emplace_back(Target(lbl));
    out.push_back(op);
    out.emplace_back(Jump(lbl));
    return true;
  }

  bool For::run(Scope &scp) {
    Coro &cor(scp.coro);
    
    if (!iter) {
      auto tgt(try_pop(cor));

      if (!tgt) {
	ERROR(Snabel, "Missing for target");
	return false;
      }

      if (!tgt->type->call) {
	ERROR(Snabel, fmt("Invalid for target: %0", *tgt));
	return false;
      }

      auto cnd(try_pop(cor));

      if (!cnd) {
	ERROR(Snabel, "Missing for condition");
	return false;
      }

      if (!cnd->type->iter) {
	ERROR(Snabel, fmt("Invalid for condition: %0", *cnd));
	return false;
      }

      iter.emplace((*cnd->type->iter)(*cnd).second);
      target.emplace(*tgt);
    }

    auto nxt((*iter)(scp.exec));
    
    if (nxt) {
      push(cor, *nxt);
      (*target->type->call)(scp, *target);
    } else {
      iter.reset();
      target.reset();
      cor.pc += 1;
    }
    
    return true;
  }

  Funcall::Funcall(Func &fn):
    OpImp(OP_FUNCALL, "funcall"), fn(fn)
  { }

  OpImp &Funcall::get_imp(Op &op) const {
    return std::get<Funcall>(op.data);
  }

  str Funcall::info() const {
    return fn.name;
  }

  bool Funcall::run(Scope &scp) {
    Coro &cor(scp.coro);
    auto imp = match(fn, cor);
    
    if (!imp) {
      ERROR(Snabel, fmt("Function not applicable: %0\n%1", 
			fn.name, curr_stack(scp.coro)));
      return false;
    }

    (*imp)(scp);
    return true;
  }
  
  Getenv::Getenv(const str &id):
    OpImp(OP_GETENV, "getenv"), id(id)
  { }

  OpImp &Getenv::get_imp(Op &op) const {
    return std::get<Getenv>(op.data);
  }

  bool Getenv::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (id.empty()) { return false; }

    if (id == "return") {
      if (scp.exec.lambdas.empty()) {
	ERROR(Snabel, "Missing lambda for return");
	return false;
      }

      auto fnd(find_env(scp, fmt("_exit%0", scp.exec.lambdas.back())));
      if (fnd) {
	out.emplace_back(Push(*fnd));
	return true;
      }
    } else {
      auto fnd(find_env(scp, id));
      if (!fnd) { return false; }
      out.emplace_back(Push(*fnd));
      return true;
    }

    return false;
  }

  bool Getenv::run(Scope &scp) {
    if (!id.empty()) {
      ERROR(Snabel, fmt("Unknown identifier: %0", id));
      return false;
    }

    auto &cor(scp.coro);
    auto &exe(scp.exec);
    auto id_arg(peek(cor));

    if (!id_arg) {
      ERROR(Snabel, "Missing identifier");
      return false;
    }

    pop(cor);

    if (id_arg->type != &exe.str_type) {
      ERROR(Snabel, fmt("Invalid identifier: %0", *id_arg));
      return false;
    }

    auto id_str(get<str>(*id_arg));
    auto fnd(find_env(scp, id_str));

    if (!fnd) {
      ERROR(Snabel, fmt("Unknown identifier: %0", id_str));
      return false;
    }
    
    push(cor, *fnd);
    return true;
  }

  Group::Group(bool copy):
    OpImp(OP_GROUP, "group"), copy(copy)
  { }

  OpImp &Group::get_imp(Op &op) const {
    return std::get<Group>(op.data);
  }

  str Group::info() const { return copy ? "copy" : ""; }

  bool Group::refresh(Scope &scp) {
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

  bool Jump::refresh(Scope &scp) {
    auto &cor(scp.coro);
    auto &exe(cor.exec);
    if (!label) { label = find_label(exe, tag); }
    return true;
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
    OpImp(OP_LAMBDA, "lambda"), exit_label(nullptr), compiled(false)
  { }

  OpImp &Lambda::get_imp(Op &op) const {
    return std::get<Lambda>(op.data);
  }

  bool Lambda::prepare(Scope &scp) {
    tag = fmt_arg(gensym(scp.exec));
    return true;
  }

  bool Lambda::refresh(Scope &scp) {
    exit_label = find_label(scp.exec, fmt("_exit%0", tag));
    scp.exec.lambdas.push_back(tag);
    return true;
  }

  bool Lambda::compile(const Op &op, Scope &scp, OpSeq &out) {
    scp.exec.lambdas.push_back(tag);
    if (compiled) { return false; }

    if (tag.empty()) {
      ERROR(Snabel, "Empty lambda tag");
      return false;
    }
    
    compiled = true;
    out.emplace_back(Jump(fmt("_skip%0", tag)));
    out.emplace_back(Target(fmt("_enter%0", tag)));
    out.emplace_back(Group(true));
    out.push_back(op);
    out.emplace_back(Target(fmt("_recall%0", tag)));
    return true;
  }

  bool Lambda::run(Scope &scp) {
    if (!exit_label) {
      ERROR(Snabel, fmt("Missing lambda exit label: %0", tag));
      return false;
    }

    scp.recall_pcs.push_back(exit_label->pc);
    return true;
  }    

  Putenv::Putenv(const str &name):
    OpImp(OP_PUTENV, "putenv"), name(name)
  { }

  OpImp &Putenv::get_imp(Op &op) const {
    return std::get<Putenv>(op.data);
  }

  str Putenv::info() const { return name; }

  bool Putenv::prepare(Scope &scp) {
    auto fnd(find_env(scp, name));
      
    if (fnd) {
      ERROR(Snabel, fmt("Duplicate env: %0", name));
      return false;
    }

    return true;
  }
  
  bool Putenv::run(Scope &scp) {
    auto &s(curr_stack(scp.coro));

    if (s.empty()) {
      ERROR(Snabel, fmt("Missing env: %0", name));
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

  bool Push::run(Scope &scp) {
    push(scp.coro, vals);
    return true;
  }

  Recall::Recall():
    OpImp(OP_RECALL, "recall"), label(nullptr)
  { }

  OpImp &Recall::get_imp(Op &op) const {
    return std::get<Recall>(op.data);
  }
  
  bool Recall::refresh(Scope &scp) {
    Coro &cor(scp.coro);
    Exec &exe(cor.exec);

    if (exe.lambdas.empty()) {
      ERROR(Snabel, "Missing lambda");
      return false;
    }
    
    auto tag = exe.lambdas.back();
    label = find_label(exe, fmt("_recall%0", tag));
    return true;
  }

  bool Recall::run(Scope &scp) {
    if (!label) {
      ERROR(Snabel, "Missing recall label");
      return false;
    }

    Coro &cor(scp.coro);
    scp.recall_pcs.push_back(cor.pc);
    jump(cor, *label);
    return true;
  }
  
  Reset::Reset():
    OpImp(OP_RESET, "reset")
  { }

  OpImp &Reset::get_imp(Op &op) const {
    return std::get<Reset>(op.data);
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

  bool Restore::run(Scope &scp) {
    restore_stack(scp.coro);
    return true;
  }

  Return::Return(bool scoped):
    OpImp(OP_RETURN, "return"), scoped(scoped)
  { }

  OpImp &Return::get_imp(Op &op) const {
    return std::get<Return>(op.data);
  }

  bool Return::refresh(Scope &scp) {
    if (scoped) { end_scope(scp.coro); }
    return true;
  }

  bool Return::run(Scope &scp) {
    Coro &cor(scp.coro);

    if (scp.recall_pcs.empty()) {
      auto &s(curr_stack(scp.coro));
      if (scoped && !end_scope(scp.coro, s.size())) { return false; }
      auto &ret_scp(curr_scope(cor));
      
      if (ret_scp.return_pc == -1) {
	ERROR(Snabel, "Missing return pc");
	return false;
      }

      cor.pc = ret_scp.return_pc;
      ret_scp.return_pc = -1;
    } else {
      cor.pc = scp.recall_pcs.back();
      scp.recall_pcs.pop_back();
    }
    
    return true;
  }
  
  Stash::Stash():
    OpImp(OP_STASH, "stash")
  { }

  OpImp &Stash::get_imp(Op &op) const {
    return std::get<Stash>(op.data);
  }

  bool Stash::run(Scope &scp) {
    Coro &cor(scp.coro);
    Exec &exe(cor.exec);
    std::shared_ptr<List> lst(new List());
    lst->swap(curr_stack(cor));

    Type *elt(lst->empty() ? &exe.any_type : lst->at(0).type);  
    for (auto i(std::next(lst->begin())); i != lst->end() && elt; i++) {
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

  Target::Target(Label &label):
    OpImp(OP_TARGET, "target"), tag(label.tag), label(&label)
  { }

  OpImp &Target::get_imp(Op &op) const {
    return std::get<Target>(op.data);
  }

  str Target::info() const {
    if (!label) { return tag; }
    return fmt("%0 (%1:%2)", label->tag, label->depth, label->pc);
  }

  bool Target::prepare(Scope &scp) {
    if (!label) {
      auto &cor(scp.coro);
      auto &exe(cor.exec);
      auto fnd(find_label(exe, tag));

      if (fnd) {
	ERROR(Snabel, fmt("Duplicate label: %0", tag));
	return false;
      }
      
      label = &add_label(exe, tag);
    }
    
    return true;
  }

  bool Target::refresh(Scope &scp) {
    if (!label) {
      ERROR(Snabel, fmt("Missing target label: %0", tag));
      return false;  
    }

    auto &cor(scp.coro);
    label->pc = cor.pc;
    label->depth = cor.scopes.size()+1;
    return true;
  }
  
  Ungroup::Ungroup():
    OpImp(OP_UNGROUP, "ungroup")
  { }

  OpImp &Ungroup::get_imp(Op &op) const {
    return std::get<Ungroup>(op.data);
  }

  bool Ungroup::refresh(Scope &scp) {
    return run(scp);
  }

  bool Ungroup::run(Scope &scp) {
    return end_scope(scp.coro);
  }

  Unlambda::Unlambda():
    OpImp(OP_UNLAMBDA, "unlambda"), compiled(false)
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

    if (tag.empty()) {
      tag = exe.lambdas.back();
    } else if (tag != exe.lambdas.back()) {
      ERROR(Snabel, "Lambda tag changed");
      return false;
    }
    
    exe.lambdas.pop_back();
    return true;
  }
  
  bool Unlambda::compile(const Op &op, Scope &scp, OpSeq &out) {
    scp.exec.lambdas.pop_back();
    if (compiled || tag.empty()) { return false; }  
    
    compiled = true;
    out.emplace_back(Target(fmt("_exit%0", tag)));
    out.push_back(op);
    out.emplace_back(Return(true));
    out.emplace_back(Target(fmt("_skip%0", tag)));
    out.emplace_back(Push(Box(scp.exec.lambda_type, fmt("_enter%0", tag))));
    return true;
  }

  bool Unlambda::run(Scope &scp) {
    if (!scp.recall_pcs.empty()) { scp.recall_pcs.pop_back(); }
    return true;
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

  bool run(Op &op, Scope &scp) {
    return op.imp.run(scp);
  }
}
