#ifndef SNABEL_PAIR_HPP
#define SNABEL_PAIR_HPP

#include "box.hpp"

namespace snabel {
  struct Exec;
  struct Type;
  
  using Pair = std::pair<Box, Box>;

  void init_pairs(Exec &exe);
  Type &get_pair_type(Exec &exe, Type &lt, Type &rt);
  str dump(const Pair &pr);
  str pair_fmt(const Pair &pr);
}

#endif
