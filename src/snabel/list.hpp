#ifndef SNABEL_LIST_HPP
#define SNABEL_LIST_HPP

#include <deque>
#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  struct ListIter: Iter {
    Type &elt;
    ListRef in;
    List::const_iterator it;
    
    ListIter(Exec &exe, Type &elt, const ListRef &in);
    opt<Box> next() override;
  };
}

#endif
