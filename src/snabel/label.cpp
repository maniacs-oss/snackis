#include "snabel/label.hpp"

namespace snabel {
  Label::Label(Exec &exe, const str &tag):
    exec(exe), tag(tag), recall(false), pc(-1), yield_depth(0)
  { }
}
