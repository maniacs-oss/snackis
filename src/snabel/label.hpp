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
    bool recall_target;
    int64_t pc;
    
    Label(Exec &exe, const str &tag);
  };
}

#endif
