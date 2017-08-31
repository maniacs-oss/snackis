#ifndef SNABEL_IO_HPP
#define SNABEL_IO_HPP

#include <cstdio>
#include <type_traits>

#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/iter.hpp"
#include "snackis/core/path.hpp"

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
    File(int fd);
    File(const Path &path, int flags);
    ~File();
  };

  template <bool rec>
  struct DirIter: Iter {
    typename std::conditional<rec, RecPathIter, PathIter>::type in;
    std::error_code err;
    
    DirIter(Exec &exe, Path in);
    opt<Box> next(Scope &scp) override;
  };

  struct ReadIter: Iter {
    Box in;
    opt<Box> out;
    Type &elt;
    
    ReadIter(Exec &exe, Type &elt, const Box &in);
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

  void init_io(Exec &exe);

  constexpr bool operator ==(const IOBuf &x, const IOBuf &y) {
    for (int64_t i(0); i < x.rpos && i < y.rpos; i++) {
      if (x.data[i] != y.data[i]) { return false; }
    }

    return true;
  }

  template <bool rec>
  DirIter<rec>::DirIter(Exec &exe, Path p):
    Iter(exe, get_iter_type(exe, exe.path_type)),
    in(p, err)
  { }
  
  template <bool rec>
  opt<Box> DirIter<rec>::next(Scope &scp) {
    if (err) {
      ERROR(Snabel, err.message());
      return nullopt;
    }
	
    if (in == snackis::end(in)) { return nullopt; }
    Box res(exec.path_type, *in);
    in++;
    return res;
  }
}

#endif
