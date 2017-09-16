#ifndef SNABEL_BOX_HPP
#define SNABEL_BOX_HPP

#include <any>
#include "snackis/core/error.hpp"
#include "snackis/core/fmt.hpp"

namespace snabel {  
  using namespace snackis;

  struct Scope;
  struct Type;
  
  using Val = std::any;
  
  struct Box {
    Type *type;
    Val val;
    int64_t safe_level;
    
    Box(Scope &scp, Type &t, const Val &v);
    Box(Scope &scp, Type &t);
  };

  using Stack = std::deque<Box>;
  
  bool operator ==(const Box &x, const Box &y);
  bool operator !=(const Box &x, const Box &y);
  bool operator <(const Box &x, const Box &y);  

  bool nil(const Box &b);
  
  template <typename T>
  const T &get(const Box &b) {
    return *std::any_cast<T>(&b.val);
  }

  template <typename T>
  T &get(Box &b) {
    return *std::any_cast<T>(&b.val);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg);
}

#endif
