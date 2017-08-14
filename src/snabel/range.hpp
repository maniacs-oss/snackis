#ifndef SNABEL_RANGE_HPP
#define SNABEL_RANGE_HPP

#include <cstdint>

namespace snabel {
  struct Range {
    int64_t beg, end;

    Range(int64_t beg, int64_t end);
  };
}

#endif
