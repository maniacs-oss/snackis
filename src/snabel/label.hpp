#ifndef SNABEL_LABEL_HPP
#define SNABEL_LABEL_HPP

#include <cstdint>
#include "snabel/sym.hpp"
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;

  struct Exec;
  
  struct Label {
    Exec &exec;
    str tag;
    bool recall;
    int64_t pc, yield_depth;
    
    Label(Exec &exe, const str &tag);
  };
}

#endif
