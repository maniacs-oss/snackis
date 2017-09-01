#ifndef SNABEL_TYPE_HPP
#define SNABEL_TYPE_HPP

#include <vector>

#include "snabel/box.hpp"
#include "snabel/iter.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {  
  using namespace snackis;

  struct Scope;

  using Types = std::vector<Type *>;
  using Conv = func<bool (Box &)>;

  enum ReadResult {READ_OK, READ_AGAIN, READ_EOF, READ_ERROR};
    
  struct Type {
    const Sym name;
    Type *raw;
    Types supers, args;
    bool conv;
    
    func<bool (const Box &, const Box &)> eq;
    func<bool (const Box &, const Box &)> equal;
    func<bool (const Box &, const Box &)> lt;
    func<bool (const Box &, const Box &)> gt;
    func<str (const Box &)> dump;
    func<str (const Box &)> fmt;
    opt<func<bool (Scope &, const Box &, bool)>> call;
    opt<func<IterRef (const Box &)>> iter;
    opt<func<ReadResult (const Box &, Bin &)>> read;
    opt<func<int64_t (const Box &, const unsigned char *, int64_t)>> write;
    
    Type(const Sym &n);
    Type(const Type &) = delete;
    ~Type();
    const Type &operator =(const Type &) = delete;
  };
  
  bool operator <(const Type &x, const Type &y);
}

#endif
