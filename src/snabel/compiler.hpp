#ifndef SNABEL_COMPILER_HPP
#define SNABEL_COMPILER_HPP

#include "snabel/op.hpp"
#include "snabel/parser.hpp"

namespace snabel {
  struct Exec;
  
  void compile(Exec &exe, TokSeq in, OpSeq &out);
}

#endif
