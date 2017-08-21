#ifndef SNABEL_RANGE_HPP
#define SNABEL_RANGE_HPP

#include <cstdint>
#include "snabel/iter.hpp"

namespace snabel {
  struct Exec;
  
  struct Range {
    int64_t beg, end;

    Range(int64_t beg, int64_t end);
  };

  struct RangeIter: Iter {
    Range in;
    
    RangeIter(Exec &exe, Range in);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
