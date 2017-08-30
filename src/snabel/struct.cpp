#include "snabel/exec.hpp"
#include "snabel/struct.hpp"

namespace snabel {
  Struct::Struct(Type &t):
    type(t)
  { }

  bool operator==(const Struct &x, const Struct &y) {
    if (x.fields.size() != y.fields.size()) { return false; }
    
    for (auto xi(x.fields.begin()), yi(y.fields.begin());
	 xi != x.fields.end() && yi != y.fields.end();
	 xi++, yi++) {
      if (*xi == *yi) { continue; }
      return false;
    }

    return true;
  }

  void init_structs(Exec &exe) {
    exe.struct_type.supers.push_back(&exe.any_type);
    exe.struct_type.fmt = [](auto &v) { return fmt_arg(*get<StructRef>(v)); };
    
    exe.struct_type.eq = [](auto &x, auto &y) {
      return get<StructRef>(x) == get<StructRef>(y);
    };

    exe.struct_type.equal = [](auto &x, auto &y) {
      return *get<StructRef>(x) == *get<StructRef>(y);
    };
  }

  Type &get_struct_type(Exec &exe, const Sym &n) {
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.struct_type;
    t.supers.push_back(&exe.struct_type);
    t.fmt = exe.struct_type.fmt;
    t.eq = exe.struct_type.eq;
    return t;
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Struct &stc) {
    OutStream out;
    out << name(stc.type.name) << '(';
    str sep("");

    for (auto [k, v]: stc.fields) {
      out << sep << name(k) << ' ' << fmt_arg(v) << '.';
      sep = " ";
    }
    
    out << ')';
    return out.str();
  }
}

