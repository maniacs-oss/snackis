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
    Iter::Ref in;
    Box target;
    
    FilterIter(Exec &exe, const Iter::Ref &in, Type &elt, const Box &tgt);
    opt<Box> next(Scope &scp) override;
  };

  struct IOQueueIter: Iter {
    IOQueueRef in;
    IOQueue::Bufs::iterator i;
    
    IOQueueIter(Exec &exe, const IOQueueRef &in);
    opt<Box> next(Scope &scp) override;
  };

  struct SplitIter: Iter {
    Iter::Ref in;
    std::set<char> chars;
    BinRef in_buf;
    Bin::iterator in_pos;
    OutStream out_buf;
    
    SplitIter(Exec &exe, const Iter::Ref &in, const std::set<char> &cs);
    opt<Box> next(Scope &scp) override;
  };

  struct MapIter: Iter {
    Iter::Ref in;
    Box target;
    
    MapIter(Exec &exe, const Iter::Ref &in, const Box &tgt);
    opt<Box> next(Scope &scp) override;
  };

  struct RandomIter: Iter {
    RandomRef in;
    Box out;
    
    RandomIter(Exec &exe, const RandomRef &in);
    opt<Box> next(Scope &scp) override;
  };
  
  struct ZipIter: Iter {
    Iter::Ref xin, yin;
    
    ZipIter(Exec &exe, const Iter::Ref &xin, const Iter::Ref &yin);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
