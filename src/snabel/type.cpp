#include <iostream>
#include "snabel/box.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Type::Type(const str &n):
    name(n)
  {
    dump = [this](auto &v) { return fmt(v); };
    equal = [this](auto &x, auto &y) { return eq(x, y); };
  }

  Type::~Type()
  { }

  bool operator <(const Type &x, const Type &y) {
    return x.name < y.name;
  }

  bool isa(const Type &x, const Type &y) {
    if (&x == &y) { return true; }
    
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
    
    for (Type *i: x.supers) {
      for (Type *j: y.supers) {
	auto res(get_super(*i, *j));
	if (res) { return res; }
      }
    }

    return nullptr;
  }
}

