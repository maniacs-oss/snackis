#ifndef SNABEL_TYPE_HPP
#define SNABEL_TYPE_HPP

#include <vector>

#include "snabel/bin.hpp"
#include "snabel/box.hpp"
#include "snabel/iter.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/str.hpp"

namespace snabel {  
  using namespace snackis;

  struct Scope;

  using Types = std::vector<Type *>;
  using Conv = func<bool (Box &, Scope &scp)>;

  enum ReadResult {READ_OK, READ_AGAIN, READ_EOF, READ_ERROR};
    
  struct Type {
    const Sym name;
    Type *raw;
    Types supers, args;
    int conv;
    
    func<bool (const Box &, const Box &)> eq;
    func<bool (const Box &, const Box &)> equal;
    func<bool (const Box &, const Box &)> lt;
    func<bool (const Box &, const Box &)> gt;
    func<void (const Box &, std::ostream &out)> uneval;
    func<str (const Box &)> dump;
    func<str (const Box &)> fmt;
    func<bool (Scope &, const Box &, bool)> call;
    opt<func<IterRef (const Box &)>> iter;
    opt<func<ReadResult (Scope &, const Box &, Bin &)>> read;
    opt<func<int64_t (Scope &, const Box &, const unsigned char *, int64_t)>> write;
    
    Type(const Sym &n);
    Type(const Type &) = delete;
    ~Type();
    const Type &operator =(const Type &) = delete;
  };
  
  bool operator <(const Type &x, const Type &y);
  void init_types(Exec &exe);
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Type &arg);
}

#endif
