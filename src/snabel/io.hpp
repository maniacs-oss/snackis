#ifndef SNABEL_IO_HPP
#define SNABEL_IO_HPP

#include <cstdio>
#include "snackis/core/path.hpp"
#include "snabel/box.hpp"
#include "snabel/iter.hpp"

namespace snabel {
  const size_t READ_BUF_SIZE(65536);
  
  struct IOBuf {
    Bin data;
    int64_t pos;
    IOBuf(int64_t size);
  };

  bool operator ==(const IOBuf &x, const IOBuf &y);

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

  struct WriteIter: Iter {
    BinRef in;
    Box out, result;
    
    WriteIter(Exec &exe, const BinRef &in, const Box &out);
    opt<Box> next(Scope &scp) override;
  };

}

#endif
