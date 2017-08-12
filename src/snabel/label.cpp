#include "snabel/label.hpp"

namespace snabel {
  Label::Label(const str &tag):
    tag(tag), depth(-1), pc(-1)
  { }
}
