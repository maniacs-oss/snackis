#ifndef SNABEL_ITERS_HPP
#define SNABEL_ITERS_HPP

#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Box;
  struct Exec;
  struct Scope;
  struct Type;
  
  struct MapIter: Iter {
    Iter::Ref in;
    Box target;
    
    MapIter(Exec &exe, const Iter::Ref &in, Type &elt, const Box &tgt);
    opt<Box> next(Scope &scp) override;
  };

  struct ZipIter: Iter {
    Iter::Ref xin, yin;
    
    ZipIter(Exec &exe, const Iter::Ref &xin, const Iter::Ref &yin);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
