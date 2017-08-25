#include "snabel/label.hpp"

namespace snabel {
  Label::Label(Exec &exe, const str &tag, bool pmt):
    exec(exe), tag(tag), permanent(pmt), recall_target(false), pc(-1), yield_depth(0)
  { }
}
