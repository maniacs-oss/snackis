#include "snabel/exec.hpp"
#include "snabel/list.hpp"

namespace snabel {
  ListIter::ListIter(Exec &exe, Type &elt, const ListRef &in, opt<Fn> fn):
    Iter(exe, get_iter_type(exe, elt)), elt(elt), in(in), it(in->begin()), fn(fn)
  { }
  
  opt<Box> ListIter::next(Scope &scp) {
    if (it == in->end()) { return nullopt; }
    auto res(*it);
    it++;
    return fn ? (*fn)(res) : res;
  }
}
