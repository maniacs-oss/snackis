#ifndef SNABEL_IO_HPP
#define SNABEL_IO_HPP

#include <cstdio>
#include "snackis/core/path.hpp"
#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  const size_t READ_BUF_SIZE(25000);
  
  struct IOBuf {
    Bin data;
    int64_t rpos;
    
    IOBuf(int64_t size);
  };

  struct File {
    Path path;
    int fd;
    File(const Path &path, int flags);
    ~File();
  };
  
  struct ReadIter: Iter {
    Box in;
    
    ReadIter(Exec &exe, const Box &in);
    opt<Box> next(Scope &scp) override;
  };

  struct WriteIter: Iter {
    IterRef in;
    BinRef in_buf;
    Box out, result;
    int64_t wpos;
    
    WriteIter(Exec &exe, const IterRef &in, const Box &out);
    opt<Box> next(Scope &scp) override;
  };

  bool operator ==(const IOBuf &x, const IOBuf &y);
}

#endif
