#include "snabel/exec.hpp"
#include "snabel/iters.hpp"
#include "snabel/pair.hpp"
#include "snabel/scope.hpp"

namespace snabel {  
  static void zip_imp(Scope &scp, const Args &args) {
    auto &l(args.at(0)), &r(args.at(1));
    
    push(scp.thread,
	 get_pair_type(scp.exec, *l.type, *r.type),
	 std::make_shared<Pair>(l, r));    
  }

  static void iterable_zip_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &x(args.at(0)), &y(args.at(1));
    auto xi((*x.type->iter)(x)), yi((*y.type->iter)(y));
    
    push(scp.thread,
	 get_iter_type(exe, get_pair_type(exe,
					  *xi->type.args.at(0),
					  *yi->type.args.at(0))),
	 IterRef(new ZipIter(exe, xi, yi)));
  }

  static void left_imp(Scope &scp, const Args &args) {
    auto &p(*get<PairRef>(args.at(0)));   
    push(scp.thread, p.first);
  }

  static void right_imp(Scope &scp, const Args &args) {
    auto &p(*get<PairRef>(args.at(0)));   
    push(scp.thread, p.second);
  }

  static void unzip_imp(Scope &scp, const Args &args) {
    auto &p(*get<PairRef>(args.at(0)));   
    push(scp.thread, p.first);
    push(scp.thread, p.second);
  }

  void init_pairs(Exec &exe) {
    exe.pair_type.supers.push_back(&exe.any_type);
    exe.pair_type.args.push_back(&exe.any_type);
    exe.pair_type.args.push_back(&exe.any_type);
    exe.pair_type.dump = [](auto &v) { return dump(*get<PairRef>(v)); };
    exe.pair_type.fmt = [](auto &v) { return pair_fmt(*get<PairRef>(v)); };
    
    exe.pair_type.eq = [](auto &x, auto &y) { 
      return get<PairRef>(x) == get<PairRef>(y); 
    };

    exe.pair_type.equal = [](auto &x, auto &y) { 
      return *get<PairRef>(x) == *get<PairRef>(y); 
    };

    exe.pair_type.lt = [](auto &x, auto &y) { 
      return *get<PairRef>(x) < *get<PairRef>(y); 
    };

    add_func(exe, ".",
	     {ArgType(exe.any_type), ArgType(exe.any_type)},
	     zip_imp);
    
    add_func(exe, "zip",
	     {ArgType(exe.iterable_type), ArgType(exe.iterable_type)},
	     iterable_zip_imp);

    add_func(exe, "left",
	     {ArgType(exe.pair_type)},
	     left_imp);

    add_func(exe, "right",
	     {ArgType(exe.pair_type)},
	     right_imp);

    add_func(exe, "unzip",
	     {ArgType(exe.pair_type)},
	     unzip_imp);
  }

  Type &get_pair_type(Exec &exe, Type &lt, Type &rt) {
    auto &thd(exe.main);
    str n(fmt("Pair<%0 %1>", lt.name, rt.name));
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
}
