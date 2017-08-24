#ifndef SNABEL_BOX_HPP
#define SNABEL_BOX_HPP

#include <deque>
#include <variant>

#include "snabel/bin.hpp"
#include "snabel/iter.hpp"
#include "snabel/uid.hpp"
#include "snackis/core/error.hpp"
#include "snackis/core/fmt.hpp"
#include "snackis/core/path.hpp"
#include "snackis/core/rat.hpp"

namespace snabel {  
  using namespace snackis;

  struct Box;
  struct Func;
  struct Exec;
  struct Proc;
  struct File;
  struct IOBuf;
  struct IOQueue;
  struct Label;
  struct Scope;
  struct Thread;
  struct Type;

  using FileRef = std::shared_ptr<File>;
  using IOBufRef = std::shared_ptr<IOBuf>;
  using IOQueueRef = std::shared_ptr<IOQueue>;
  using List = std::deque<Box>;
  using ListRef = std::shared_ptr<List>;
  using Pair = std::pair<Box, Box>;
  using PairRef = std::shared_ptr<Pair>;
  using ProcRef = std::shared_ptr<Proc>;
  
  struct Empty
  { };
  
  using Val = std::variant<Empty, bool, Byte, char, int64_t, Path, Rat, str, uchar,
			   Uid, ustr,
			   BinRef, FileRef, Iter::Ref, IOBufRef, IOQueueRef, ListRef,
			   PairRef, ProcRef, 
			   Func *, Label *, Thread *, Type *>;
  
  struct Box {
    Type *type;
    Val val;

    Box(Type &t, const Val &v);
  };

  using Stack = std::deque<Box>;
  extern Empty empty_val;
  
  bool operator ==(const Box &x, const Box &y);
  bool operator !=(const Box &x, const Box &y);
  str dump(const List &lst);
  str list_fmt(const List &lst);
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
