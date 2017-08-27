#ifndef SNABEL_ITER_HPP
#define SNABEL_ITER_HPP

#include <memory>
#include "snackis/core/opt.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Box;
  struct Exec;
  struct Scope;
  struct Type;
  
  struct Iter {    
    Exec &exec;
    Type &type;
    
    Iter(Exec &exec, Type &type);
    virtual opt<Box> next(Scope &scp) = 0;
  };

  using IterRef = std::shared_ptr<Iter>;
}

#endif
