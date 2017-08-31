#include "snabel/exec.hpp"
#include "snabel/str.hpp"

namespace snabel {
  StrIter::StrIter(Exec &exe, const StrRef &in):
    Iter(exe, get_iter_type(exe, exe.char_type)), in(in), i(in->begin())
  { }

  opt<Box> StrIter::next(Scope &scp){
    if (i == in->end()) { return nullopt; }
    auto res(*i);
    i++;
    return Box(exec.char_type, res);
  }

  UStrIter::UStrIter(Exec &exe, const UStrRef &in):
    Iter(exe, get_iter_type(exe, exe.uchar_type)), in(in), i(in->begin())
  { }
  
  opt<Box> UStrIter::next(Scope &scp){
    if (i == in->end()) { return nullopt; }
    auto res(*i);
    i++;
    return Box(exec.uchar_type, res);
  }
}
