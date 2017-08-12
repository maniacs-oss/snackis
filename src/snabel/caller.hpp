#ifndef SNABEL_CALLER_HPP
#define SNABEL_CALLER_HPP

#include <cstdint>

namespace snabel {
  struct Caller {
    int64_t pc;
    bool recall;
    
    Caller(int64_t pc, bool recall);
  };
}

#endif
