#ifndef SNABEL_STR_HPP
#define SNABEL_STR_HPP

#include "snabel/iter.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct StrIter: Iter {
    StrRef in;
    str::const_iterator i;
    
    StrIter(Exec &exe, const StrRef &in);
    opt<Box> next(Scope &scp) override;
  };

  struct UStrIter: Iter {
    UStrRef in;
    ustr::const_iterator i;
    
    UStrIter(Exec &exe, const UStrRef &in);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
