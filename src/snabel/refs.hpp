#ifndef SNABEL_REFS_HPP
#define SNABEL_REFS_HPP

#include <memory>
#include "snackis/core/str.hpp"

namespace snabel{
  using namespace snackis;
  
  struct File;
  
  using FileRef = std::shared_ptr<File>;
  using StrRef = std::shared_ptr<str>;
  using UStrRef = std::shared_ptr<ustr>;
}

#endif
