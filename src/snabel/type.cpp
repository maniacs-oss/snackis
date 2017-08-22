#include <iostream>
#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Type::Type(const str &n):
    name(n), raw(this)
  {
    dump = [this](auto &v) { return fmt(v); };
    fmt = [n](auto &v) { return n; };
    eq = [](auto &x, auto &y) { return false; };
    equal = [this](auto &x, auto &y) { return eq(x, y); };
  }

  Type::~Type()
  { }

  bool operator <(const Type &x, const Type &y) {
    return x.name < y.name;
  }

  bool isa(const Types &x, const Types &y) {
    auto i(x.begin()), j(y.begin());

    for (; i != x.end() && j != y.end(); i++, j++) {
      if (!isa(**i, **j)) { return false; }
    }
    
    return i == x.end() && j == y.end();
  }
  
  bool isa(const Type &x, const Type &y) {
    if (&x == &y || (x.raw == y.raw && isa(x.args, y.args))) { return true; }
    
    for (Type *xs: x.supers) {
      if (isa(*xs, y)) { return true; }
    }

    return false;
  }

  bool isa(const Box &val, const Type &typ) {
    return isa(*val.type, typ);
  }

  Type *get_super(Type &x, Type &y) {
    if (&x == &y) { return &x; }

    for (auto i(x.supers.rbegin()); i != x.supers.rend(); i++) {
      for (auto j(y.supers.rbegin()); j != y.supers.rend(); j++) {
	auto res(get_super(**i, **j));
	if (res) { return res; }
      }
    }

    return nullptr;
  }
}

