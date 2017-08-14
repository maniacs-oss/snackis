#ifndef SNABEL_ITER_HPP
#define SNABEL_ITER_HPP

#include <variant>
#include "snabel/box.hpp"
#include "snabel/range.hpp"

namespace snabel {
  struct Scope;

  struct Iter {
    using Cond = std::variant<Range>;
    
    Cond cond;
    Box target;    

    func<opt<Box> (Cond &, Scope &)> next;

    Iter(const Cond &cond, const Box &target);
  };

  bool next(Iter &iter, Scope &scp);
}

#endif
