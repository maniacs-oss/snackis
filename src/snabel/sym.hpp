#ifndef SNABEL_SYM_HPP
#define SNABEL_SYM_HPP

#include <cstdint>
#include "snackis/core/str.hpp"

namespace snabel {
  struct Exec;
  
  struct Sym {
    Exec &exec;
    const str &name;
    
    Sym(Exec &exe, const str &n);
    ~Sym();
  };

  using SymRef = std::shared_ptr<Sym>;
}

#endif
