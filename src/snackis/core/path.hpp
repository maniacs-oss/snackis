#ifndef SNACKIS_PATH_HPP
#define SNACKIS_PATH_HPP

#include <experimental/filesystem>

namespace snackis {
  namespace stdfs = std::experimental::filesystem;

  using Path = stdfs::path;
  using PathIter = stdfs::directory_iterator;
  using RecPathIter = stdfs::recursive_directory_iterator;
  
  bool create_path(const Path &p);
  bool path_exists(const Path &p);
  bool remove_path(const Path &p);
  bool is_file(const Path &p);
  PathIter end(const PathIter &it);
  RecPathIter end(const RecPathIter &it);
}

#endif
