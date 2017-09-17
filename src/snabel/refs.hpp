#ifndef SNABEL_REFS_HPP
#define SNABEL_REFS_HPP

#include <memory>
#include "snackis/core/str.hpp"

namespace snabel{
  using namespace snackis;
  
  struct File;
  struct Lambda;
  
  using FileRef = std::shared_ptr<File>;
  using LambdaRef = std::shared_ptr<Lambda>;
  using StrRef = std::shared_ptr<str>;
  using UStrRef = std::shared_ptr<ustr>;
}

#endif
