#ifndef SNABEL_SYM_HPP
#define SNABEL_SYM_HPP

#include <map>
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Exec;
  struct Sym;

  using SymTable = std::map<str, Sym>;

  struct Sym {
    using Pos = size_t;
    
    SymTable::iterator it;
    Pos *pos;
    
    Sym(Pos *pos);
  };

  void init_syms(Exec &exe);
  const Sym &get_sym(Exec &exe, const str &n);
  const str &name(const Sym &sym);

  constexpr bool operator ==(const Sym &x, const Sym &y) {
    return x.pos == y.pos;
  }

  constexpr bool operator <(const Sym &x, const Sym &y) {
    return *x.pos < *y.pos;
  }
}

#endif
