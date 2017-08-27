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
      if (empty(v)) { return "#n/a"; }
      return fmt("Opt(%0)", v.type->args.at(0)->dump(v));
    };
    
    exe.opt_type.fmt = [](auto &v) -> str {
      if (empty(v)) { return "#n/a"; }
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

    put_env(exe.main_scope, "#n/a", Box(exe.opt_type, empty_val));

    add_func(exe, "opt",
	     {ArgType(exe.any_type)},
	     {ArgType([&exe](auto &args) {
		   return &get_opt_type(exe, *args.at(0).type);
		 })},
	     opt_imp);

    add_func(exe, "or",
	     {ArgType(exe.opt_type),
		 ArgType([](auto &args) { return args.at(0).type->args.at(0); })},
	     {ArgType([](auto &args) { return args.at(0).type->args.at(0); })},
	     or_imp);

    add_func(exe, "or",
	     {ArgType(exe.opt_type), ArgType(0)}, {ArgType(0)},
	     or_opt_imp);

    add_func(exe, "unopt",
	     {ArgType(get_iterable_type(exe, exe.opt_type))},
	     {ArgType([&exe](auto &args) {
		   auto &elt(*args.at(0).type->args.at(0));
		   return &get_iter_type(exe, *elt.args.at(0));
		 })},
	     unopt_iterable_imp);
  }

  Type &get_opt_type(Exec &exe, Type &elt) {    
    str n(fmt("Opt<%0>", elt.name));
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
