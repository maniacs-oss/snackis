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

  IOQueue::IOQueue():
    len(0), wpos(0)
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
    res = (*out.type->write)(out, &b[in->wpos], b.size()-in->wpos);
    if (res == -1) { return nullopt; }
    in->wpos += res;
    in->len -= res;
    
    if (in->wpos == b.size()) {
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

  bool push(IOQueue &q, const IOBuf &buf){
    if (!buf.rpos) { return false; }
    q.bufs.emplace_back(buf.data.begin(), std::next(buf.data.begin(), buf.rpos));
    q.len += buf.rpos;
    return true;
  }
  
  bool push(IOQueue &q, const Bin &bin) {
    if (bin.empty()) { return false; }
    q.bufs.push_back(bin);
    q.len += bin.size();
    return true;
  }			       					 
}
