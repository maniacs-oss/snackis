#include "snabel/label.hpp"

namespace snabel {
  Label::Label(Exec &exe, const str &tag, bool pmt):
    exec(exe), tag(tag), permanent(pmt), pc(-1),
    recall_depth(0), yield_depth(0), break_depth(0)
  { }
}
