#ifndef SNABEL_PAIR_HPP
#define SNABEL_PAIR_HPP

namespace snabel {
  struct Exec;
  struct Type;
  
  void init_pairs(Exec &exe);
  Type &get_pair_type(Exec &exe, Type &lt, Type &rt);
}

#endif
