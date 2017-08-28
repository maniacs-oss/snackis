#include <iostream>
#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Type::Type(const str &n):
    name(n), raw(this), conv(false)
  {
    dump = [this](auto &v) { return fmt(v); };
    fmt = [n](auto &v) { return n; };
    eq = [](auto &x, auto &y) { return false; };
    equal = [this](auto &x, auto &y) { return eq(x, y); };
    gt = [this](auto &x, auto &y) { return !lt(x, y) && !eq(x, y); };
  }

  Type::~Type()
  { }

  bool operator <(const Type &x, const Type &y) {
    return x.name < y.name;
  }
}

