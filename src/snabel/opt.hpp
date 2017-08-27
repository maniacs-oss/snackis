#ifndef SNABEL_OPT_HPP
#define SNABEL_OPT_HPP

#include "snabel/iter.hpp"

namespace snabel {
  struct OptIter: Iter {
    IterRef in;
    
    OptIter(Exec &exe, Type &elt, const IterRef &in);
    opt<Box> next(Scope &scp) override;
  };

  void init_opts(Exec &exe);
  Type &get_opt_type(Exec &exe, Type &elt);
}

#endif
