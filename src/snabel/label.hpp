#ifndef SNABEL_LABEL_HPP
#define SNABEL_LABEL_HPP

#include <cstdint>
#include "snabel/sym.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Label {
    str tag;
    bool recall;
    Label *yield_target;
    int64_t pc;
    
    Label(const str &tag);
  };
}

#endif
