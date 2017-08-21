#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/iters.hpp"
#include "snabel/type.hpp"

namespace snabel {
  MapIter::MapIter(Exec &exe, const Iter::Ref &in, Type &elt, const Box &tgt):
    Iter(exe, get_iter_type(exe, elt)),
    in(in), target(tgt)
  { }
  
  opt<Box> MapIter::next(Scope &scp) {
    auto &thd(scp.thread);
    auto x(in->next(scp));
    if (!x) { return nullopt; }
    push(thd, *x);
    (*target.type->call)(scp, target);
    auto out(peek(thd));
    
    if (!out) {
      ERROR(Snabel, "Map iterator target failed");
      return nullopt;
    }
    
    return *out;
  }

  ZipIter::ZipIter(Exec &exe, const Iter::Ref &xin, const Iter::Ref &yin):
    Iter(exe, get_iter_type(exe, get_pair_type(exe,
					       *xin->type.args.at(0),
					       *yin->type.args.at(0)))),
    xin(xin), yin(yin)
  { }
  
  opt<Box> ZipIter::next(Scope &scp) {
    auto x(xin->next(scp)), y(yin->next(scp));
    if (!x || !y) { return nullopt; }
    return Box(get_pair_type(exec, *x->type, *y->type),
	       std::make_shared<Pair>(*x, *y));
  }
}
