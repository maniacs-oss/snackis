#include "snabel/exec.hpp"
#include "snabel/rat.hpp"
#include "snackis/core/rat.hpp"

namespace snabel {
  static void div_i64_imp(Scope &scp, const Args &args) {
    auto num(get<int64_t>(args.at(0)));
    auto div(get<int64_t>(args.at(1)));
    bool neg = (num < 0 && div > 0) || (div < 0 && num >= 0);
    push(scp, scp.exec.rat_type, Rat(abs(num), abs(div), neg));
  }

  static void frac_imp(Scope &scp, const Args &args) {
    push(scp, scp.exec.rat_type, frac(get<Rat>(args.at(0))));
  }

  static void add_rat_imp(Scope &scp, const Args &args) {
    auto x(get<Rat>(args.at(0)));
    auto y(get<Rat>(args.at(1)));
    push(scp, scp.exec.rat_type, x+y);
  }

  static void sub_rat_imp(Scope &scp, const Args &args) {
    auto x(get<Rat>(args.at(0)));
    auto y(get<Rat>(args.at(1)));
    push(scp, scp.exec.rat_type, x-y);
  }

  static void mul_rat_imp(Scope &scp, const Args &args) {
    auto x(get<Rat>(args.at(0)));
    auto y(get<Rat>(args.at(1)));
    push(scp, scp.exec.rat_type, x*y);
  }

  static void div_rat_imp(Scope &scp, const Args &args) {
    auto x(get<Rat>(args.at(0)));
    auto y(get<Rat>(args.at(1)));
    push(scp, scp.exec.rat_type, x/y);
  }

  void init_rats(Exec &exe) {
    exe.rat_type.supers.push_back(&exe.any_type);
    exe.rat_type.supers.push_back(&exe.ordered_type);
    exe.rat_type.fmt = [](auto &v) { return fmt_arg(get<Rat>(v)); };
    exe.rat_type.eq = [](auto &x, auto &y) { return get<Rat>(x) == get<Rat>(y); };
    exe.rat_type.lt = [](auto &x, auto &y) { return get<Rat>(x) < get<Rat>(y); };

    add_conv(exe, exe.i64_type, exe.rat_type, [&exe](auto &v) {	
	v.type = &exe.rat_type;
	auto n(get<int64_t>(v));
	v.val = Rat(abs(n), 1, n < 0);
	return true;
      });

    add_func(exe, "/", Func::Pure,
	     {ArgType(exe.i64_type), ArgType(exe.i64_type)},
	     div_i64_imp);
    
    add_func(exe, "frac", Func::Pure, {ArgType(exe.rat_type)}, frac_imp);

    add_func(exe, "+", Func::Pure,
	     {ArgType(exe.rat_type), ArgType(exe.rat_type)},
	     add_rat_imp);

    add_func(exe, "-", Func::Pure,
	     {ArgType(exe.rat_type), ArgType(exe.rat_type)},
	     sub_rat_imp);
    
    add_func(exe, "*", Func::Pure,
	     {ArgType(exe.rat_type), ArgType(exe.rat_type)},
	     mul_rat_imp);
    
    add_func(exe, "/", Func::Pure,
	     {ArgType(exe.rat_type), ArgType(exe.rat_type)},
	     div_rat_imp);	
  }
}
