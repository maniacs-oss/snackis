#include "snabel/exec.hpp"
#include "snabel/str.hpp"

namespace snabel {
  StrIter::StrIter(Exec &exe, const str &in):
    Iter(exe, get_iter_type(exe, exe.char_type)), in(in), i(0)
  { }
  
  opt<Box> StrIter::next(Scope &scp){
    if (i == in.size()) { return nullopt; }
    auto res(in[i]);
    i++;
    return Box(exec.char_type, res);
  }
}
