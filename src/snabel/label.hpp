#ifndef SNABEL_LABEL_HPP
#define SNABEL_LABEL_HPP

#include <cstdint>
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;

  struct Exec;

  struct Label {
    Exec &exec;
    str tag;
    bool permanent, recall_target;
    int64_t pc, yield_depth, break_depth;
    
    Label(Exec &exe, const str &tag, bool pmt);
  };
}

#endif
