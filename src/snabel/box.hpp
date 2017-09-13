#ifndef SNABEL_BOX_HPP
#define SNABEL_BOX_HPP

#include <deque>
#include <map>
#include <variant>

#include "snabel/bin.hpp"
#include "snabel/iter.hpp"
#include "snabel/random.hpp"
#include "snabel/sym.hpp"
#include "snabel/uid.hpp"
#include "snackis/core/error.hpp"
#include "snackis/core/fmt.hpp"
#include "snackis/core/path.hpp"
#include "snackis/core/rat.hpp"

namespace snabel {  
  using namespace snackis;

  struct Box;
  struct Coro;
  struct Func;
  struct Exec;
  struct Proc;
  struct File;
  struct IOBuf;
  struct IOQueue;
  struct Label;
  struct Lambda;
  struct Scope;
  struct Struct;
  struct Thread;
  struct Type;

  using CoroRef = std::shared_ptr<Coro>;
  using FileRef = std::shared_ptr<File>;
  using IOBufRef = std::shared_ptr<IOBuf>;
  using IOQueueRef = std::shared_ptr<IOQueue>;
  using LambdaRef = std::shared_ptr<Lambda>;
  using List = std::deque<Box>;
  using ListRef = std::shared_ptr<List>;
  using Pair = std::pair<Box, Box>;
  using PairRef = std::shared_ptr<Pair>;
  using ProcRef = std::shared_ptr<Proc>;
  using StrRef = std::shared_ptr<str>;
  using StructRef = std::shared_ptr<Struct>;
  using Table = std::map<Box, Box>;
  using TableRef = std::shared_ptr<Table>;
  using UStrRef = std::shared_ptr<ustr>;
    
  struct Nil
  { };
  
  using Val = std::variant<Nil, bool, Byte, char, int64_t, Path, Rat, uchar, Uid,
			   BinRef, CoroRef, FileRef, IterRef, IOBufRef, IOQueueRef,
			   LambdaRef, ListRef, PairRef, ProcRef, RandomRef, StrRef,
			   StructRef, TableRef, UStrRef,
			   Func *, Label *, Sym, Thread *, Type *>;
  
  struct Box {
    Type *type;
    Val val;
    int64_t safe_level;
    
    Box(Scope &scp, Type &t, const Val &v);
  };

  using Stack = std::deque<Box>;
  extern Nil nil;
  
  bool operator ==(const Box &x, const Box &y);
  bool operator !=(const Box &x, const Box &y);
  bool operator <(const Box &x, const Box &y);  

  str dump(const List &lst);
  str list_fmt(const List &lst);
  str dump(const Table &tbl);
  str table_fmt(const Table &tbl);
  str dump(const Pair &pr);
  str pair_fmt(const Pair &pr);
  bool empty(const Box &b);
  
  template <typename T>
  const T &get(const Box &b) {
    CHECK(std::holds_alternative<T>(b.val), _);
    return std::get<T>(b.val);
  }

  template <typename T>
  T &get(Box &b) {
    CHECK(std::holds_alternative<T>(b.val), _);
    return std::get<T>(b.val);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg);
}

#endif
