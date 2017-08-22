#include "snabel/exec.hpp"
#include "snabel/bin.hpp"

namespace snabel {
  BinIter::BinIter(Exec &exe, const BinRef &in):
    Iter(exe, get_iter_type(exe, exe.byte_type)), in(in), i(in->begin())
  { }
  
  opt<Box> BinIter::next(Scope &scp){
    if (i == in->end()) { return nullopt; }
    auto res(*i);
    i++;
    return Box(exec.byte_type, res);
  }
}
