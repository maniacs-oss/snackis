#include "snabel/label.hpp"

namespace snabel {
  Label::Label(const str &tag):
    tag(tag), recall(false), yield_target(nullptr), pc(-1)
  { }
}
