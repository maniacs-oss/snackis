#ifndef SNABEL_MACRO_HPP
#define SNABEL_MACRO_HPP

#include "snabel/op.hpp"
#include "snabel/parser.hpp"
#include "snackis/core/func.hpp"

namespace snabel {
  const int MAX_MACRO_ARGS(10);
  
  struct Macro {
    using Imp = func<void (Pos, TokSeq &, OpSeq &)>;

    Sym name;
    Imp imp;
    int nargs;
    
    Macro(Sym name, Imp imp);
    Macro(Exec &exe, Sym name, int nargs, const LambdaRef &lmb);
    void operator ()(Pos pos, TokSeq &in, OpSeq &out) const;
  };
}

#endif
