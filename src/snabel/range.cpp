#include "snabel/box.hpp"
#include "snabel/exec.hpp"
#include "snabel/range.hpp"

namespace snabel {
  Range::Range(int64_t beg, int64_t end):
    beg(beg), end(end)
  { }

  RangeIter::RangeIter(Exec &exe, Range in):
    Iter(exe, get_iter_type(exe, exe.i64_type)), in(in)
  { }
  
  opt<Box> RangeIter::next(Scope &scp) {
    if (in.beg == in.end) { return nullopt; }
    auto res(in.beg);
    in.beg++;
    return Box(exec.i64_type, res);		
  }
}
