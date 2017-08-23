#ifndef SNABEL_TYPE_HPP
#define SNABEL_TYPE_HPP

#include <deque>
#include "snabel/box.hpp"
#include "snabel/iter.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {  
  using namespace snackis;

  struct Scope;

  using Types = std::deque<Type *>;
  using Conv = func<bool (Box &)>;
  
  struct Type {
    const str name;
    Type *raw;
    Types supers, args;
    
    func<bool (const Box &, const Box &)> eq;
    func<bool (const Box &, const Box &)> equal;
    func<str (const Box &)> dump;
    func<str (const Box &)> fmt;
    opt<func<bool (Scope &, const Box &, bool)>> call;
    opt<func<Iter::Ref (const Box &)>> iter;
    opt<func<bool (const Box &, Bin &)>> read;
    opt<func<int64_t (const Box &, const unsigned char *, size_t)>> write;
    
    Type(const str &n);
    Type(const Type &) = delete;
    ~Type();
    const Type &operator =(const Type &) = delete;
  };
  
  bool operator <(const Type &x, const Type &y);
  bool isa(const Types &x, const Types &y);
  bool isa(const Type &x, const Type &y);
  bool isa(const Box &val, const Type &typ);
  Type *get_super(Type &x, Type &y);
}

#endif
