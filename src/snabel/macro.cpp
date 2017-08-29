#include "snabel/macro.hpp"

namespace snabel {
  Macro::Macro(Imp imp):
    imp(imp)
  { }

  void Macro::operator ()(Pos pos, TokSeq &in, OpSeq &out) const {
    imp(pos, in, out);
  }
}
