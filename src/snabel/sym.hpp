#ifndef SNABEL_SYM_HPP
#define SNABEL_SYM_HPP

#include "snackis/core/str.hpp"

namespace snabel {
  struct Exec;
  struct Sym;

  using SymTable = std::map<str, Sym>;

  struct Sym {
    Exec &exec;
    SymTable::iterator it;
    size_t pos;
    
    Sym(Exec &exe);
  };

  bool operator ==(const Sym &x, const Sym &y);
  bool operator <(const Sym &x, const Sym &y);
  
  void init_syms(Exec &exe);
  Sym &get_sym(Exec &exe, const str &n);
  const str &name(const Sym &sym);
}

#endif
