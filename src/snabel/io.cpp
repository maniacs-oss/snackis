#include <fcntl.h>
#include <unistd.h>
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"

namespace snabel {
  File::File(const Path &path, int flags):
    path(path), fd(open(path.c_str(), flags | O_NONBLOCK))
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
    out(exe.bin_type, std::make_shared<Bin>())
  { }
  
  opt<Box> ReadIter::next(Scope &scp){
    auto &bin(*get<BinRef>(out));
    bin.resize(READ_BUF_SIZE);
    if (!(*in.type->read)(in, bin)) { return nullopt; }
    return out;
  }

  WriteIter::WriteIter(Exec &exe, const BinRef &in, const Box &buf):
    Iter(exe, get_iter_type(exe, exe.i64_type)),
    in(in), buf(buf), out(exe.i64_type, (int64_t)0)
  { }
  
  opt<Box> WriteIter::next(Scope &scp){
    auto &bin(*in);
    if (bin.empty()) { return nullopt; }
    auto res((*out.type->write)(out, &bin[0], bin.size()));
    if (res == -1) { return nullopt; }
    bin.erase(bin.begin(), std::next(bin.begin(), res));
    get<int64_t>(out) += res;
    return out;
  }
}
