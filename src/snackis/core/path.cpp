#include "snackis/core/path.hpp"

namespace snackis {  
  bool create_path(const Path &p) {
    return stdfs::create_directories(p);
  }

  bool path_exists(const Path &p) {
    return stdfs::exists(p);
  }

  bool remove_path(const Path &p) {
    std::error_code e;
    stdfs::remove_all(p, e);
    return e.value() == 0;
  }

  bool is_file(const Path &p) {
    return stdfs::is_regular_file(p);
  }

  PathIter end(const PathIter &it) {
    return stdfs::end(it);
  }

  RecPathIter end(const RecPathIter &it) {
    return stdfs::end(it);
  }
}
