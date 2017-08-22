#ifndef SNABEL_BOX_HPP
#define SNABEL_BOX_HPP

#include <deque>
#include <variant>

#include "snabel/bin.hpp"
#include "snabel/iter.hpp"
#include "snackis/core/error.hpp"
#include "snackis/core/fmt.hpp"
#include "snackis/core/path.hpp"
#include "snackis/core/rat.hpp"

namespace snabel {  
  using namespace snackis;

  struct Box;
  struct Func;
  struct Exec;
  struct Fiber;
  struct File;
  struct Label;
  struct Scope;
  struct Thread;
  struct Type;

  using FiberRef = std::shared_ptr<Fiber>;
  using FileRef = std::shared_ptr<File>;
  using List = std::deque<Box>;
  using ListRef = std::shared_ptr<List>;
  using Pair = std::pair<Box, Box>;
  using PairRef = std::shared_ptr<Pair>;
  
  struct Empty
  { };
  
  using Val = std::variant<Empty, bool, Byte, char, int64_t, Path, Rat, str,
			   BinRef, FiberRef, FileRef, Iter::Ref, ListRef, PairRef, 
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
