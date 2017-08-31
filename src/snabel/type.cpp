#include <iostream>
#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Type::Type(const Sym &n):
    name(n), raw(this), conv(false)
  {
    dump = [](auto &v) { return v.type->fmt(v); };
    fmt = [](auto &v) { return snabel::name(v.type->name); };
    eq = [](auto &x, auto &y) { return false; };
    equal = [](auto &x, auto &y) { return x.type->eq(x, y); };
    
    gt = [](auto &x, auto &y) {
      auto &t(*x.type);
      return !t.lt(x, y) && !t.eq(x, y);
    };
  }

  Type::~Type()
  { }

  bool operator <(const Type &x, const Type &y) {
    return x.name < y.name;
  }
}

