#include "snabel/exec.hpp"
#include "snabel/iters.hpp"
#include "snabel/pair.hpp"
#include "snabel/scope.hpp"

namespace snabel {  
  static void zip_imp(Scope &scp, const Args &args) {
    auto &l(args.at(0)), &r(args.at(1));
    
    push(scp,
	 get_pair_type(scp.exec, *l.type, *r.type),
	 std::make_pair(l, r));    
  }

  static void iterable_zip_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &x(args.at(0)), &y(args.at(1));
    auto xi((*x.type->iter)(x)), yi((*y.type->iter)(y));
    
    push(scp,
	 get_iter_type(exe, get_pair_type(exe,
					  *xi->type.args.at(0),
					  *yi->type.args.at(0))),
	 IterRef(new ZipIter(exe, xi, yi)));
  }

  static void left_imp(Scope &scp, const Args &args) {
    auto &p(get<Pair>(args.at(0)));   
    push(scp.thread, p.first);
  }

  static void right_imp(Scope &scp, const Args &args) {
    auto &p(get<Pair>(args.at(0)));   
    push(scp.thread, p.second);
  }

  static void unzip_imp(Scope &scp, const Args &args) {
    auto &p(get<Pair>(args.at(0)));   
    push(scp.thread, p.first);
    push(scp.thread, p.second);
  }

  void init_pairs(Exec &exe) {
    exe.pair_type.supers.push_back(&exe.any_type);
    exe.pair_type.args.push_back(&exe.any_type);
    exe.pair_type.args.push_back(&exe.any_type);
    exe.pair_type.dump = [](auto &v) { return dump(get<Pair>(v)); };
    exe.pair_type.fmt = [](auto &v) { return pair_fmt(get<Pair>(v)); };
    
    exe.pair_type.eq = [](auto &x, auto &y) { 
      return get<Pair>(x) == get<Pair>(y); 
    };

    exe.pair_type.lt = [](auto &x, auto &y) { 
      return get<Pair>(x) < get<Pair>(y); 
    };

    add_func(exe, ".", Func::Const,
	     {ArgType(exe.any_type), ArgType(exe.any_type)},
	     zip_imp);
    
    add_func(exe, "zip", Func::Const,
	     {ArgType(exe.iterable_type), ArgType(exe.iterable_type)},
	     iterable_zip_imp);

    add_func(exe, "left", Func::Pure,
	     {ArgType(exe.pair_type)},
	     left_imp);

    add_func(exe, "right", Func::Pure,
	     {ArgType(exe.pair_type)},
	     right_imp);

    add_func(exe, "unzip", Func::Pure,
	     {ArgType(exe.pair_type)},
	     unzip_imp);
  }

  Type &get_pair_type(Exec &exe, Type &lt, Type &rt) {
    auto &thd(curr_thread(exe));
    auto &n(get_sym(exe, fmt("Pair<%0 %1>", name(lt.name), name(rt.name))));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    
    auto &t(add_type(exe, n));
    t.raw = &exe.pair_type;
    t.supers.push_back(&exe.any_type);

    if (isa(thd, lt, exe.ordered_type) && isa(thd, rt, exe.ordered_type)) {
      t.supers.push_back(&exe.ordered_type);
    }

    t.supers.push_back(&exe.pair_type);
    t.args.push_back(&lt);
    t.args.push_back(&rt);

    t.dump = exe.pair_type.dump;
    t.fmt = exe.pair_type.fmt;
    t.eq = exe.pair_type.eq;
    t.equal = exe.pair_type.equal;
    t.lt = exe.pair_type.lt;
    return t;
  }

  str dump(const Pair &pr) {
    auto &l(pr.first), &r(pr.second);
    return fmt("%0 %1.", l.type->dump(l), r.type->dump(r));
  }

  str pair_fmt(const Pair &pr) {
    auto &l(pr.first), &r(pr.second);
    return fmt("%0 %1.", l.type->fmt(l), r.type->fmt(r));
  }
}
