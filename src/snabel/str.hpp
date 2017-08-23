#ifndef SNABEL_STR_HPP
#define SNABEL_STR_HPP

#include "snabel/iter.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct StrIter: Iter {
    const str in;
    size_t i;
    StrIter(Exec &exe, const str &in);
    opt<Box> next(Scope &scp) override;
  };

  struct UStrIter: Iter {
    const ustr in;
    size_t i;
    
    UStrIter(Exec &exe, const ustr &in);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
