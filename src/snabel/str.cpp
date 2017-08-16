#include "snabel/exec.hpp"
#include "snabel/str.hpp"

namespace snabel {
  StrIter::StrIter(Exec &exe, const str &in):
    Iter(exe, get_iter_type(exe, exe.char_type)), in(in), it(this->in.begin())
  { }
  
  bool StrIter::ready() const {
    return it != in.end();
  }
  
  opt<Box> StrIter::next(){
    if (it == in.end()) { return nullopt; }
    auto res(*it);
    it++;
    return Box(exec.char_type, res);
  }
}
