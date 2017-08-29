#ifndef SNABEL_MACRO_HPP
#define SNABEL_MACRO_HPP

#include "snabel/op.hpp"
#include "snabel/parser.hpp"
#include "snackis/core/func.hpp"

namespace snabel {
  struct Macro {
    using Imp = func<void (Pos, TokSeq &, OpSeq &)>;
    Imp imp;
    
    Macro(Imp imp);
    void operator ()(Pos pos, TokSeq &in, OpSeq &out) const;
  };
}

#endif
