#ifndef SNABEL_TYPE_HPP
#define SNABEL_TYPE_HPP

#include <deque>
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {  
  using namespace snackis;

  struct Box;
  
  struct Type {    
    const str name;
    Type *super;
    std::deque<Type *> args;
    
    func<bool (const Box &, const Box &)> eq;
    func<str (const Box &)> fmt;
    
    Type(const str &n);
    Type(const str &n, Type &super);
    Type(const Type &) = delete;
    virtual ~Type();
    const Type &operator =(const Type &) = delete;
  };
  
  bool operator <(const Type &x, const Type &y);
  bool isa(const Box &val, const Type &typ);
  Type *get_super(Type &x, Type &y);
}

#endif
