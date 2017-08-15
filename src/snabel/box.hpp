#ifndef SNABEL_BOX_HPP
#define SNABEL_BOX_HPP

#include <deque>
#include <variant>

#include "snackis/core/error.hpp"
#include "snackis/core/fmt.hpp"

namespace snabel {  
  using namespace snackis;

  struct Box;
  struct Func;
  struct Exec;
  struct Label;
  struct List;
  struct Scope;
  struct Thread;
  struct Type;

  struct Undef {
    bool operator ()(const Box &box) const;
  };
  
  using ListRef = std::shared_ptr<List>;

  using Iter = func<opt<Box> (Exec &)>;
  using IterRef = std::shared_ptr<Iter>;
  
  using Val = std::variant<Undef,
			   bool, char, int64_t, str,
			   ListRef, IterRef,
			   Func *, Label *, Thread *, Type *>;
  
  struct Box {
    Type *type;
    Val val;

    Box(Type &t, const Val &v);
  };

  struct List {
    std::deque<Box> elems;
  };

  using Stack = std::deque<Box>;

  extern const Undef undef;

  bool operator ==(const Box &x, const Box &y);
  bool operator !=(const Box &x, const Box &y);
  IterRef make_iter(const Box &in);
  ListRef make_list();
  
  template <typename T>
  const T &get(const Box &b) {
    CHECK(!undef(b), _);
    CHECK(std::holds_alternative<T>(b.val), _);
    return std::get<T>(b.val);
  }

  template <typename T>
  T &get(Box &b) {
    CHECK(!undef(b), _);
    CHECK(std::holds_alternative<T>(b.val), _);
    return std::get<T>(b.val);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg);
}

#endif
