#ifndef SNABEL_LABEL_HPP
#define SNABEL_LABEL_HPP

#include <cstdint>
#include "snabel/sym.hpp"

namespace snabel {
  using namespace snackis;

  struct Exec;

  struct Label {
    Exec &exec;
    const Sym tag;
    bool permanent, push_result;
    int64_t pc, return_depth, recall_depth, yield_depth, break_depth;
    
    Label(Exec &exe, const Sym &tag, bool pmt);
  };
}

#endif
