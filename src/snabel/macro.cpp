#include "snabel/macro.hpp"

namespace snabel {
  Macro::Macro(const str &n, Imp imp):
    name(n), imp(imp)
  { }

  void Macro::operator ()(TokSeq &in, OpSeq &out) const {
    imp(in, out);
  }
}
