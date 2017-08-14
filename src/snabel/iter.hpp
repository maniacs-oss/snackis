#ifndef SNABEL_ITER_HPP
#define SNABEL_ITER_HPP

#include <variant>
#include "snabel/box.hpp"
#include "snabel/range.hpp"

namespace snabel {
  struct Scope;

  struct Iter {
    Box target;    
    func<opt<Box> (Scope &)> next;
    Iter(const Box &target);
  };

  bool next(Iter &iter, Scope &scp);
}

#endif
