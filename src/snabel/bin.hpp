#ifndef SNABEL_BIN_HPP
#define SNABEL_BIN_HPP

#include <vector>
#include "snabel/iter.hpp"

namespace snabel {
  using Byte = uint8_t;
  using Bin = std::vector<Byte>;
  using BinRef = std::shared_ptr<Bin>;

  struct BinIter: Iter {
    BinRef in;
    Bin::const_iterator i;
    
    BinIter(Exec &exe, const BinRef &in);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
