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
    
    //Sym(const Sym &src);
    //const Sym &operator =(const Sym &);

    Sym(Pos *pos);
  };

  bool operator ==(const Sym &x, const Sym &y);
  bool operator <(const Sym &x, const Sym &y);
  
  void init_syms(Exec &exe);
  const Sym &get_sym(Exec &exe, const str &n);
  const str &name(const Sym &sym);
}

#endif
