#ifndef SNABEL_ITER_HPP
#define SNABEL_ITER_HPP

#include <variant>
#include "snabel/box.hpp"
#include "snabel/range.hpp"

namespace snabel {
  struct Scope;

  struct Iter {
    using Next = func<opt<Box> (Scope &)>;
    
    Box target;    
    Next next;

    Iter(const Box &target, Next next);
  };

  bool next(Iter &iter, Scope &scp);
}

#endif
