#include "snabel/exec.hpp"
#include "snabel/list.hpp"

namespace snabel {
  ListIter::ListIter(Exec &exe, Type &elt, const ListRef &in):
    Iter(exe, get_iter_type(exe, elt)), elt(elt), in(in), it(in->begin())
  { }
  
  opt<Box> ListIter::next(){
    if (it == in->end()) { return nullopt; }
    auto res(*it);
    it++;
    return res;
  }
}
