#ifndef SNABEL_IO_HPP
#define SNABEL_IO_HPP

#include <cstdio>
#include "snackis/core/path.hpp"
#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  const size_t READ_BUF_SIZE(65536);
  
  struct File {
    Path path;
    int fd;
    File(const Path &path, int flags);
    ~File();
  };
  
  struct ReadIter: Iter {
    Box in, out;
    
    ReadIter(Exec &exe, const Box &in);
    opt<Box> next(Scope &scp) override;
  };
}

#endif
