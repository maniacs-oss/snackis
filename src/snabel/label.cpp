#include "snabel/label.hpp"

namespace snabel {
  Label::Label(Exec &exe, const str &tag):
    exec(exe), tag(tag), recall_target(false), pc(-1)
  { }
}
