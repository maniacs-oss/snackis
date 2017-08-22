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
}
