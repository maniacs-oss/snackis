#ifndef SNABEL_ITERS_HPP
#define SNABEL_ITERS_HPP

#include <set>
#include "snabel/box.hpp"
#include "snabel/iter.hpp"
#include "snabel/io.hpp"
#include "snabel/random.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Box;
  struct Exec;
  struct Scope;
  struct Type;
  
  struct FilterIter: Iter {
    IterRef in;
    Box target;
    
    FilterIter(Exec &exe, const IterRef &in, Type &elt, const Box &tgt);
    opt<Box> next(Scope &scp) override;
  };

  struct SplitIter: Iter {
    IterRef in;
    std::set<char> chars;
    BinRef in_buf;
    Bin::iterator in_pos;
    OutStream out_buf;
    
    SplitIter(Exec &exe, const IterRef &in, const std::set<char> &cs);
    opt<Box> next(Scope &scp) override;
  };

  struct MapIter: Iter {
    IterRef in;
    Box target;
    
    MapIter(Exec &exe, const IterRef &in, const Box &tgt);
    opt<Box> next(Scope &scp) override;
  };

  struct RandomIter: Iter {
    RandomRef in;
    Box out;
    
    RandomIter(Exec &exe, const RandomRef &in);
    opt<Box> next(Scope &scp) override;
  };
  
  struct ZipIter: Iter {
    IterRef xin, yin;
    
    ZipIter(Exec &exe, const IterRef &xin, const IterRef &yin);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
