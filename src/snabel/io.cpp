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

  ReadIter::ReadIter(Exec &exe, Type &elt, const Box &in):
    Iter(exe, get_iter_type(exe, elt)),
    in(in), elt(elt)
  { }
  
  opt<Box> ReadIter::next(Scope &scp) {
    if (!out) { out.emplace(elt, std::make_shared<Bin>(READ_BUF_SIZE)); }
    auto res((*in.type->read)(in, *get<BinRef>(*out)));
    
    switch (res) {
    case READ_OK: {
      auto res(out);
      out.reset();
      return res;
    }
    case READ_AGAIN:
      return Box(elt, empty_val);  
    case READ_EOF:
    case READ_ERROR:
      return nullopt;
    default:
      ERROR(Snabel, fmt("Invalid read result: %0", res));
    }

    return nullopt;
  }

  WriteIter::WriteIter(Exec &exe, const IterRef &in, const Box &out):
    Iter(exe, get_iter_type(exe, exe.i64_type)),
    in(in), out(out), result(exe.i64_type, (int64_t)-1), wpos(0)
  { }
  
  opt<Box> WriteIter::next(Scope &scp){
    if (!in_buf) {
      auto nxt(in->next(scp));
      if (!nxt) { return nullopt; }
      in_buf = get<BinRef>(*nxt);
    }

    auto &b(*in_buf);
    auto &res(get<int64_t>(result));
    res = (*out.type->write)(out, &b[wpos], b.size()-wpos);
    if (res == -1) { return nullopt; }
    wpos += res;
    
    if (wpos == b.size()) {
      in_buf.reset();
      wpos = 0;
    }
    
    return result;
  }

  bool operator ==(const IOBuf &x, const IOBuf &y) {
    for (int64_t i(0); i < x.rpos && i < y.rpos; i++) {
      if (x.data[i] != y.data[i]) { return false; }
    }

    return true;
  }
}
