#ifndef SNABEL_BOX_HPP
#define SNABEL_BOX_HPP

#include <variant>

#include "snackis/core/error.hpp"
#include "snackis/core/fmt.hpp"

namespace snabel {  
  using namespace snackis;

  struct Box;
  struct Func;
  struct Exec;
  struct Type;

  struct Undef {
    bool operator ()(const Box &box) const;
  };

  using Val = std::variant<const Undef, bool, int64_t, str, Func *, Type *>;
  
  struct Box {
    Type *type;
    Val val;

    Box(Type &t, const Val &v);
  };

  extern const Undef undef;

  bool operator ==(const Box &x, const Box &y);
  bool operator !=(const Box &x, const Box &y);
  
  template <typename T>
  T get(const Box &b) {
    CHECK(!undef(b), _);
    return std::get<T>(b.val);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg);
}

#endif
