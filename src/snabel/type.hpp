#ifndef SNABEL_TYPE_HPP
#define SNABEL_TYPE_HPP

#include <deque>
#include "snabel/box.hpp"
#include "snabel/iter.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {  
  using namespace snackis;

  struct Box;
  struct Scope;
  
  struct Type {    
    const str name;
    std::deque<Type *> supers, args;
    
    func<bool (const Box &, const Box &)> eq;
    func<bool (const Box &, const Box &)> equal;
    func<str (const Box &)> dump;
    func<str (const Box &)> fmt;
    opt<func<bool (Scope &, const Box &)>> call;
    opt<func<Iter::Ref (const Box &)>> iter;
    
    Type(const str &n);
    Type(const Type &) = delete;
    virtual ~Type();
    const Type &operator =(const Type &) = delete;
  };
  
  bool operator <(const Type &x, const Type &y);
  bool isa(const Type &x, const Type &y);
  bool isa(const Box &val, const Type &typ);
  Type *get_super(Type &x, Type &y);
}

#endif
