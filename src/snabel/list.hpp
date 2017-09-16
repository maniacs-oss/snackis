#ifndef SNABEL_LIST_HPP
#define SNABEL_LIST_HPP

#include <deque>
#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  using List = std::deque<Box>;
  using ListRef = std::shared_ptr<List>;

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

  void init_lists(Exec &exe);
  Type &get_list_type(Exec &exe, Type &elt);
  void uneval(const List &lst, std::ostream &out);
  str dump(const List &lst);
  str list_fmt(const List &lst);
}

#endif
