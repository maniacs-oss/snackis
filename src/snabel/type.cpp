#include "snabel/box.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Type::Type(const str &n):
    name(n), super(nullptr)
  { }

  Type::Type(const str &n, Type &super):
    name(n), super(&super)
  { }

  Type::~Type()
  { }

  bool operator <(const Type &x, const Type &y) {
    return x.name < y.name;
  }

  bool isa(const Box &val, const Type &typ) {
    for (Type *i = val.type; i; i = i->super) {
      if (i == &typ) { return true; }
    }

    return false;
  }

  Type *get_super(Type &x, Type &y) {
    for (Type *i = &x; i; i = i->super) {
      for (Type *j = &y; j; j = j->super) {
	if (i == j) { return i; }
      }
    }

    return nullptr;
  }
}
