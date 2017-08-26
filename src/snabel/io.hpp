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

  struct IOQueue {
    using Bufs = std::deque<Bin>;
    Bufs bufs;
    int64_t len, wpos;
    IOQueue();
  };

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
    IOQueueRef in;
    Box out, result;
    
    WriteIter(Exec &exe, const IOQueueRef &in, const Box &out);
    opt<Box> next(Scope &scp) override;
  };

  bool operator ==(const IOBuf &x, const IOBuf &y);
  bool operator ==(const IOQueue &x, const IOQueue &y);
  bool push(IOQueue &q, const IOBuf &buf);
  bool push(IOQueue &q, const Bin &bin);
}

#endif
