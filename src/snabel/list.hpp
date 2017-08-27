#ifndef SNABEL_LIST_HPP
#define SNABEL_LIST_HPP

#include <deque>
#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  struct ListIter: Iter {
    using Fn = func<Box (const Box &)>;
    
    Type &elt;
    ListRef in;
    List::const_iterator it;
    opt<Fn> fn;
    
    ListIter(Exec &exe, Type &elt, const ListRef &in, opt<Fn> fn=nullopt);
    opt<Box> next(Scope &scp) override;
  };

  struct FifoIter: Iter {
    ListRef in;
    
    FifoIter(Exec &exe, Type &elt, const ListRef &in);
    opt<Box> next(Scope &scp) override;
  };

}

#endif
