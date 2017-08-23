#include <fcntl.h>
#include <unistd.h>
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"

namespace snabel {
  IOBuf::IOBuf(int64_t size):
    pos(0)
  {
    data.reserve(size);
  }

  bool operator ==(const IOBuf &x, const IOBuf &y) {
    for (int64_t i(0); i < x.pos && i < y.pos; i++) {
      if (x.data[i] != y.data[i]) { return false; }
    }

    return true;
  }

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
    out(exe.iobuf_type, std::make_shared<IOBuf>(READ_BUF_SIZE))
  { }
  
  opt<Box> ReadIter::next(Scope &scp){
    auto &buf(*get<IOBufRef>(out));
    buf.pos = 0;
    if (!(*in.type->read)(in, buf)) { return nullopt; }
    return out;
  }

  WriteIter::WriteIter(Exec &exe, const BinRef &in, const Box &out):
    Iter(exe, get_iter_type(exe, exe.i64_type)),
    in(in), out(out), result(exe.i64_type, (int64_t)-1)
  { }
  
  opt<Box> WriteIter::next(Scope &scp){
    auto &bin(*in);
    if (bin.empty()) { return nullopt; }
    auto &res(get<int64_t>(result));
    res = (*out.type->write)(out, &bin[0], bin.size());
    if (res == -1) { return nullopt; }
    if (res) {
      if (res == bin.size()) {
	bin.clear();
      } else {
	bin.erase(bin.begin(), std::next(bin.begin(), res));
      }
    }
    return result;
  }
}
