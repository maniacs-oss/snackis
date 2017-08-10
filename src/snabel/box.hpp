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
  struct List;
  struct Type;

  struct Undef {
    bool operator ()(const Box &box) const;
  };
  
  using ListRef = std::shared_ptr<List>;
    
  using Val = std::variant<const Undef,
			   bool, int64_t, str,
			   ListRef,
			   Func *, Type *>;
  
  struct Box {
    Type *type;
    Val val;

    Box(Type &t, const Val &v);
  };

  struct List {
    std::deque<Box> elems;
  };

  extern const Undef undef;

  bool operator ==(const Box &x, const Box &y);
  bool operator !=(const Box &x, const Box &y);
  
  template <typename T>
  const T &get(const Box &b) {
    CHECK(!undef(b), _);
    return std::get<T>(b.val);
  }

  template <typename T>
  T &get(Box &b) {
    CHECK(!undef(b), _);
    return std::get<T>(b.val);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg);
}

#endif
