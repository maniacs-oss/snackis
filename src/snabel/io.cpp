#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"

namespace snabel {
  IOBuf::IOBuf(int64_t size):
    data(size), rpos(0)
  { }

  IOBuf::IOBuf(const Bin &in):
    data(in), rpos(in.size())
  { }

  IOQueue::IOQueue():
    wpos(0)
  { }
  
  File::File(const Path &path, int flags):
    path(path), fd(open(path.c_str(), flags | O_NONBLOCK, 0666))
  {
    if (fd == -1) {
      ERROR(Snabel, fmt("Failed opening file '%0': %1", path.string(), errno));
    }
  }

  File::~File() {
    if (fd != -1) { close(fd); }
  }

  ReadIter::ReadIter(Exec &exe, const Box &in):
    Iter(exe, get_iter_type(exe, exe.bin_type)),
    in(in),
    out(exe.io_buf_type, std::make_shared<IOBuf>(READ_BUF_SIZE))
  { }
  
  opt<Box> ReadIter::next(Scope &scp){
    auto &buf(*get<IOBufRef>(out));
    buf.rpos = 0;
    if (!(*in.type->read)(in, buf)) { return nullopt; }
    return out;
  }

  WriteIter::WriteIter(Exec &exe, const IOQueueRef &in, const Box &out):
    Iter(exe, get_iter_type(exe, exe.i64_type)),
    in(in), out(out), result(exe.i64_type, (int64_t)-1)
  { }
  
  opt<Box> WriteIter::next(Scope &scp){
    auto &q(in->bufs);
    if (q.empty()) { return nullopt; }
    auto &res(get<int64_t>(result));
    auto &b(q.front());
    res = (*out.type->write)(out, &b.data[in->wpos], b.rpos-in->wpos);
    if (res == -1) { return nullopt; }
    in->wpos += res;
      
    if (in->wpos == b.rpos) {
      q.pop_front();
      in->wpos = 0;
    }
    
    return result;
  }

  bool operator ==(const IOBuf &x, const IOBuf &y) {
    for (int64_t i(0); i < x.rpos && i < y.rpos; i++) {
      if (x.data[i] != y.data[i]) { return false; }
    }

    return true;
  }

  bool operator ==(const IOQueue &x, const IOQueue &y) {
    for (int64_t i(0); i < x.bufs.size() && i < y.bufs.size(); i++) {
      if (x.bufs[i] == y.bufs[i]) { continue; }
      return false;
    }

    return true;
  }
}
