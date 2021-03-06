#include <algorithm>
#include <iostream>

#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/func.hpp"
#include "snabel/lambda.hpp"
#include "snabel/list.hpp"
#include "snabel/op.hpp"
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

  Begin::Begin(Exec &exe):
    OpImp(OP_BEGIN, "begin"),
    tag(uid(exe)),
    enter_label(add_label(exe, fmt("_enter%0", tag))),
    skip_label(add_label(exe, fmt("_skip%0", tag))),
    compiled(false)
  {
    begin_scope(curr_thread(exe));
  }

  OpImp &Begin::get_imp(Op &op) const {
    return std::get<Begin>(op.data);
  }

  bool Begin::refresh(Scope &scp) {
    auto &exe(scp.exec);
    exe.lambdas.push_back(this);
    return false;
  }

  bool Begin::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }
    compiled = true;
    out.emplace_back(Jump(skip_label));
    out.emplace_back(Target(enter_label));
    out.push_back(op);
    return true;
  }

  bool Begin::run(Scope &scp) {
    auto &thd(scp.thread);

    Scope &new_scp(begin_scope(thd));
    new_scp.target = &enter_label;
    new_scp.recall_pc = thd.pc+1;
    
    if (scp.coro) {
      auto &cor(scp.coro);
      thd.pc = cor->pc;
      new_scp.safe_level = cor->safe_level;
      std::copy(cor->stacks.begin(), cor->stacks.end(),
		std::back_inserter(thd.stacks));
      cor->op_state.swap(new_scp.op_state);
      cor->env.swap(new_scp.env);
      cor->on_exit.swap(new_scp.on_exit);
    } else {
      if (!thd.lambda) {
	ERROR(Snabel, "Missing lambda");
	return false;
      }
      
      auto &lmb(*thd.lambda);
      new_scp.safe_level = lmb.safe_level;
	       
      std::copy(lmb.env.begin(), lmb.env.end(),
		std::inserter(scp.env, scp.env.end()));
    }

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

  Capture::Capture(Label &tgt):
    OpImp(OP_CAPTURE, "capture"), target(tgt)
  { }

  OpImp &Capture::get_imp(Op &op) const {
    return std::get<Capture>(op.data);
  }

  bool Capture::run(Scope &scp) {
    push(scp, scp.exec.lambda_type, std::make_shared<Lambda>(target, scp));
    return true;
  }

  Defconv::Defconv(Type &from, Type &to):
    OpImp(OP_DEFCONV, "defconv"), from(from), to(to)
  { }

  OpImp &Defconv::get_imp(Op &op) const {
    return std::get<Defconv>(op.data);
  }

  str Defconv::info() const { return fmt("%0->%1", from.name, to.name); }

  bool Defconv::run(Scope &scp) {
    auto &exe(scp.exec);
    auto lmb(try_pop(scp.thread));

    if (!lmb) {
      ERROR(Snabel, fmt("Missing conv lamda: %0->%1", from.name, to.name));
      return false;
    }

    auto lmbr(get<LambdaRef>(*lmb));
    
    auto fnd(exe.convs.find(std::make_pair(&from, &to)));
    opt<Conv> prev;
    if (fnd != exe.convs.end()) { prev = fnd->second; }
    
    auto hnd(add_conv(exe, from, to, [this, &exe, lmbr](auto &v, auto &scp) {
	push(scp.thread, v);
	call(lmbr, scp, true);
	auto out(try_pop(scp.thread));
	
	if (!out) {
	  ERROR(Snabel, fmt("Conversion failed for %0: %1->%2",
			    v, from.name, to.name));
	  return false;
	}

	if (nil(*out)) { return false; }
	v = *out;
	v.type = v.type->args.at(0);
	return true;
	}));

    scp.on_exit.push_back([&exe, hnd, prev](auto &scp) {
	if (prev) {
	  hnd->second = *prev;
	} else {
	  rem_conv(exe, hnd);
	}
      });

    return true;
  }

  Defunc::Defunc(const Sym &name, const ArgTypes &args):
    OpImp(OP_DEFUNC, "defunc"), name(name), args(args)
  { }

  OpImp &Defunc::get_imp(Op &op) const {
    return std::get<Defunc>(op.data);
  }

  str Defunc::info() const { return snabel::name(name); }

  bool Defunc::run(Scope &scp) {
    auto lmb(try_pop(scp.thread));

    if (!lmb) {
      ERROR(Snabel, fmt("Missing function lamda: %0", snabel::name(name)));
      return false;
    }

    auto hnd(add_func(scp.exec, name, Func::Safe, args, get<LambdaRef>(*lmb)));
    scp.on_exit.push_back([hnd](auto &scp) { rem_func(hnd); });
    return true;
  }

  Deref::Deref(const Sym &name):
    OpImp(OP_DEREF, "deref"), name(name), compiled(false)
  { }

  OpImp &Deref::get_imp(Op &op) const {
    return std::get<Deref>(op.data);
  }

  str Deref::info() const { return snabel::name(name); }

  bool Deref::prepare(Scope &scp) {
    auto &exe(scp.exec);
    str in(snabel::name(name));
    auto i(in.find('<'));

    if (i != str::npos) {
      name = get_sym(exe, in.substr(0, i));
      i++;
      
      while (i < in.size()) {
	auto res(parse_type(exe, in, i));
	if (!res.first) { break; }
	i = res.second;
	args.push_back(res.first);
      }
    }

    return true;
  }
  
  bool Deref::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }
    compiled = true;
    auto &exe(scp.exec);
    auto fnd(find_env(scp, name));
    if (!fnd) { return false; }

    if (fnd->type == &exe.func_type) {
      out.emplace_back(Funcall(*get<Func *>(*fnd), args));
      return true;
    }
    
    return false;
  }

  bool Deref::run(Scope &scp) {
    auto fnd(find_env(scp, name));
    
    if (!fnd) {
      ERROR(UnknownId, name);
      return false;
    }

    if (fnd->type == &scp.exec.func_type) {
      TRY(try_call);
      auto &fn(*get<Func *>(*fnd));
      auto m(match(fn, args, scp));
      
      if (!m) {
	ERROR(FuncApp, fn, curr_stack(scp.thread));
	return false;
      }

      if (!try_call.errors.empty()) { return false; }
      return (*m->first)(scp, m->second);
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

  Dump::Dump():
    OpImp(OP_DUMP, "dump")
  { }

  OpImp &Dump::get_imp(Op &op) const {
    return std::get<Dump>(op.data);
  }

  bool Dump::run(Scope &scp) {
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
    
    push(scp, get_list_type(exe, elt ? *elt : exe.any_type), lst);
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

  End::End(Exec &exe):
    OpImp(OP_END, "end"),
    enter_label(nullptr), skip_label(nullptr),
    compiled(false)
  {
    end_scope(curr_thread(exe));
  }

  OpImp &End::get_imp(Op &op) const {
    return std::get<End>(op.data);
  }

  bool End::refresh(Scope &scp) {
    Exec &exe(scp.exec);
    
    if (exe.lambdas.empty()) {
      ERROR(Snabel, "Missing lambda");
      return false;
    }

    auto &l(*exe.lambdas.back());
    enter_label = &l.enter_label;
    skip_label = &l.skip_label;
    exe.lambdas.pop_back();
    return false;
  }
  
  bool End::compile(const Op &op, Scope &scp, OpSeq &out) {
    if (compiled) { return false; }  
    
    compiled = true;
    out.push_back(op);
    out.emplace_back(Target(*skip_label));
    out.emplace_back(Capture(*enter_label));
    return true;
  }

  bool End::run(Scope &scp) {
    return _return(scp, 1, scp.push_result);
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
	  ERROR(Snabel, fmt("Format failed, not enough values on stack: %0\n%1",
			    in.substr(s.first+offs),
			    stack));
	  return false;
	}
	
	auto &v(stack.at(stack.size()-i-1));

	if (v.safe_level != scp.safe_level) {
	  ERROR(UnsafeStack);
	  return false;
	}

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
    
    push(scp, scp.exec.str_type, std::make_shared<str>(out));
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

  Funcall::Funcall(Func &fn, const Types &args):
    OpImp(OP_FUNCALL, "funcall"), fn(fn), imp(nullptr), args(args)
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
    TRY(try_funcall);
    auto &thd(scp.thread);
    
    if (imp) {
      auto m(match(*imp, args, scp, true));

      if (m) {
	(*imp)(scp, *m);
	return true;
      }
    }

    auto m(match(fn, args, scp));
    
    if (!m) {
      ERROR(FuncApp, fn, curr_stack(thd));
      return false;
    }

    imp = m->first;

    if (scp.safe_level && imp->sec >= Func::Unsafe) {
      ERROR(UnsafeCall, fn);
      return false;
    }

    if (!try_funcall.errors.empty()) { return false; }
    return(*imp)(scp, m->second);
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
    for (auto &v: vals) { v.safe_level = scp.safe_level; }
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

  Return::Return(int64_t dep, bool push_result):
    OpImp(OP_RETURN, "return"), target(nullptr), depth(dep), push_result(push_result)
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
    target = &l->enter_label;
    return false;
  }

  bool Return::run(Scope &scp) {
    return _return(scp, depth, push_result);
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
  
  Test::Test(const str &msg):
    OpImp(OP_TEST, "test"), msg(msg)
  { }

  OpImp &Test::get_imp(Op &op) const {
    return std::get<Test>(op.data);
  }

  bool Test::run(Scope &scp) {
    auto cnd(try_pop(scp.thread));
    
    if (!cnd) {
      ERROR(Test, fmt("Missing test condition: %0", msg));
      return false;
    }

    if (!get<bool>(*cnd)) {
      ERROR(Test, fmt("Test failed: %0", msg));
      return false;
    }

    return true;
  }

  Yield::Yield(int64_t depth, bool push_result):
    OpImp(OP_YIELD, "yield"), depth(depth), push_result(push_result)
  { }

  OpImp &Yield::get_imp(Op &op) const {
    return std::get<Yield>(op.data);
  }

  str Yield::info() const { return fmt_arg(depth); }

  bool Yield::run(Scope &scp) {
    return yield(scp, depth, push_result);
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
