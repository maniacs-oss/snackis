#include <iostream>
#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Type::Type(const Sym &n):
    name(n), raw(this), conv(0)
  {
    uneval = [this](auto &v, auto &out) { out << dump(v); };
    dump = [this](auto &v) { return fmt(v); };
    fmt = [](auto &v) { return fmt_arg(v.type->name); };
    eq = [](auto &x, auto &y) { return false; };
    equal = [this](auto &x, auto &y) { return eq(x, y); };
      
    gt = [](auto &x, auto &y) {
      auto &t(*x.type);
      return !t.lt(x, y) && !t.eq(x, y);
    };

    call = [](auto &scp, auto &v, auto now) {
      push(scp.thread, v);
      return true;
    };
  }

  Type::~Type()
  { }

  bool operator <(const Type &x, const Type &y) {
    return x.name < y.name;
  }

  void init_types(Exec &exe) {
    add_macro(exe, "type:", [&exe](auto pos, auto &in, auto &out) {
	if (in.size() < 3 || in.at(2).text != ";") {
	  ERROR(Snabel, fmt("Malformed type definition on row %0, col %1",
			    pos.row, pos.col));
	  return;
	} else {
	  auto &n(in.at(0).text);
	  in.pop_front();

	  if (!isupper(n.at(0))) {
	    ERROR(Snabel, fmt("Type name should be capitalized: %0", n));
	    return;
	  }

	  auto &tn(in.at(0).text);
	  in.pop_front();
	  auto t(parse_type(exe, tn, 0).first);
	  
	  if (!t) {
	    ERROR(Snabel, fmt("Invalid super type: %0", tn));
	    return;
	  }

	  auto &scp(curr_scope(exe));
	  put_env(scp, n, Box(scp, get_meta_type(exe, *t), t));
	  in.pop_front();
	}
      });
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Type &arg) {
    return fmt_arg(arg.name);
  }
}
