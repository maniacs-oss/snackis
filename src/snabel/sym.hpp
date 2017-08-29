#ifndef SNABEL_SYM_HPP
#define SNABEL_SYM_HPP

#include <cstdint>
#include <memory>
#include "snackis/core/str.hpp"

namespace snabel {
  struct Exec;
  struct Sym;

  using SymTable = std::map<str, std::weak_ptr<Sym>>;

  struct Sym {
    Exec &exec;
    SymTable::iterator it;
    size_t pos;
    
    Sym(Exec &exe, SymTable::iterator it);
    ~Sym();
  };

  using SymRef = std::shared_ptr<Sym>;

  bool operator ==(const Sym &x, const Sym &y);
  bool operator <(const Sym &x, const Sym &y);
  
  void init_syms(Exec &exe);
  SymRef get_sym(Exec &exe, const str &n);
  const str &name(const Sym &sym);
}

#endif
