#include "snabel/macro.hpp"

namespace snabel {
  Macro::Macro(const str &n, Imp imp):
    name(n), imp(imp)
  { }

  void Macro::operator ()(Pos pos, TokSeq &in, OpSeq &out) const {
    imp(pos, in, out);
  }
}
