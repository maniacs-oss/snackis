#include "snabel/label.hpp"

namespace snabel {
  Label::Label(const str &tag):
    tag(tag), recall(false), pc(-1)
  { }
}
