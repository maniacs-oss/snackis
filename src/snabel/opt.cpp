#include "snabel/exec.hpp"
#include "snabel/opt.hpp"

namespace snabel {
  OptIter::OptIter(Exec &exe, Type &elt, const IterRef &in):
    Iter(exe, get_iter_type(exe, elt)), in(in)
  { }
  
  opt<Box> OptIter::next(Scope &scp) {
    while (in) {
      auto nxt(in->next(scp));

      if (!nxt) { 
	in.reset();
	break; 
      }

      if (!empty(*nxt)) { return Box(*nxt->type->args.at(0), nxt->val); }
    }

    return nullopt;
  }

  static void opt_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, get_opt_type(scp.exec, *in.type), in.val);
  }

  static void when_imp(Scope &scp, const Args &args) {
    auto &cnd(args.at(0)), &tgt(args.at(1));
    
    if (!empty(cnd)) { 
      push(scp.thread, *cnd.type->args.at(0), cnd.val);
      tgt.type->call(scp, tgt, false); 
    }
  }
  
  static void unless_imp(Scope &scp, const Args &args) {
    auto &cnd(args.at(0)), &tgt(args.at(1));
    if (empty(cnd)) { tgt.type->call(scp, tgt, false); }
  }

  static void if_imp(Scope &scp, const Args &args) {
    auto &cnd(args.at(0)), &left(args.at(1)), &right(args.at(2));
    
    if (empty(cnd)) {
      right.type->call(scp, right, false);       
    } else { 
      push(scp.thread, *cnd.type->args.at(0), cnd.val);
      left.type->call(scp, left, false); 
    }
  }
  
  static void or_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto &alt(args.at(1));

    if (empty(in)) {
      push(scp.thread, alt);
    } else {
      push(scp.thread, *in.type->args.at(0), in.val);
    }
  }

  static void or_opt_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto &alt(args.at(1));
    push(scp.thread, empty(in) ? alt : in);
  }

  static void unopt_iterable_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0)->args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 IterRef(new OptIter(exe, elt, it)));
  }

  
  void init_opts(Exec &exe) {
    exe.opt_type.supers.push_back(&exe.any_type);
    exe.opt_type.args.push_back(&exe.any_type);

    exe.opt_type.dump = [](auto &v) -> str {
      if (empty(v)) { return "nil"; }
      return fmt("Opt(%0)", v.type->args.at(0)->dump(v));
    };
    
    exe.opt_type.fmt = [](auto &v) -> str {
      if (empty(v)) { return "nil"; }
      return fmt("Opt(%0)", v.type->args.at(0)->fmt(v));
    };

    exe.opt_type.eq = [](auto &x, auto &y) {
      if (empty(x) && !empty(y)) { return true; }
      if (!empty(x) || !empty(y)) { return false; }
      return x.type->eq(x, y);
    };

    exe.opt_type.equal = [](auto &x, auto &y) {
      if (empty(x) && !empty(y)) { return true; }
      if (!empty(x) || !empty(y)) { return false; }
      return x.type->equal(x, y);
    };

    add_func(exe, "opt", Func::Const, 
	     {ArgType(exe.any_type)},
	     opt_imp);

    add_func(exe, "when", Func::Pure,
	     {ArgType(exe.opt_type), ArgType(exe.any_type)},
	     when_imp);

    add_func(exe, "unless", Func::Pure,
	     {ArgType(exe.opt_type), ArgType(exe.any_type)},
	     unless_imp);

    add_func(exe, "if", Func::Pure,
	     {ArgType(exe.opt_type), ArgType(exe.any_type), ArgType(exe.any_type)},
	     if_imp);

    add_func(exe, "or", Func::Pure,
	     {ArgType(exe.opt_type),
		 ArgType([](auto &args) { return args.at(0).type->args.at(0); })},
	     or_imp);

    add_func(exe, "or", Func::Pure,
	     {ArgType(exe.opt_type), ArgType(0)},
	     or_opt_imp);

    add_func(exe, "unopt", Func::Const,
	     {ArgType(get_iterable_type(exe, exe.opt_type))},
	     unopt_iterable_imp);

    add_macro(exe, "nil", [&exe](auto pos, auto &in, auto &out) {
	out.emplace_back(Push(Box(exe.opt_type, nil)));
      });    
  }

  Type &get_opt_type(Exec &exe, Type &elt) {    
    auto &n(get_sym(exe, fmt("Opt<%0>", name(elt.name))));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.opt_type;
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&exe.opt_type);
    t.args.push_back(&elt);
    t.fmt = exe.opt_type.fmt;
    t.dump = exe.opt_type.dump;
    t.eq = exe.opt_type.eq;
    t.equal = exe.opt_type.equal;
    return t;
  }
}
