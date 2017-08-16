#ifndef SNABEL_ITER_HPP
#define SNABEL_ITER_HPP

#include <memory>
#include "snackis/core/opt.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Box;
  struct Exec;
  struct Type;
  
  struct Iter {
    using Ref = std::shared_ptr<Iter>;
    
    Exec &exec;
    Type &type;
    
    Iter(Exec &exec, Type &type);
    virtual opt<Box> next() = 0;
  };  

  struct ZipIter: Iter {
    Iter::Ref xin, yin;
    
    ZipIter(Exec &exe, const Iter::Ref &xin, const Iter::Ref &yin);
    opt<Box> next() override;
  };
}

#endif
