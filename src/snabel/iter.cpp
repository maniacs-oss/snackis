#include "snabel/exec.hpp"
#include "snabel/iter.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Iter::Iter(Exec &exec, Type &type):
    exec(exec), type(type)
  { }

  ZipIter::ZipIter(Exec &exe, const Iter::Ref &xin, const Iter::Ref &yin):
    Iter(exe, get_iter_type(exe, get_pair_type(exe,
					       *xin->type.args.at(0),
					       *yin->type.args.at(0)))),
    xin(xin), yin(yin)
  { }
  
  bool ZipIter::ready() const {
    return xin->ready() && yin->ready();
  }
  
  opt<Box> ZipIter::next() {
    auto x(xin->next()), y(yin->next());
    if (!x || !y) { return nullopt; }
    return Box(get_pair_type(exec, *x->type, *y->type),
	       std::make_shared<Pair>(*x, *y));
  }
}
