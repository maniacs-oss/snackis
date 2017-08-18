#ifndef SNABEL_LABEL_HPP
#define SNABEL_LABEL_HPP

#include <cstdint>
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Label {
    str tag;
    bool recall;
    int64_t pc;
    
    Label(const str &tag);
  };
}

#endif
