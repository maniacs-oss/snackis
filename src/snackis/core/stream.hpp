#ifndef SNACKIS_STREAM_HPP
#define SNACKIS_STREAM_HPP

#include <sstream>
#include "snackis/core/char.hpp"

namespace snackis {
  using Stream = std::stringstream;
  using InStream = std::istringstream;
  using OutStream = std::ostringstream;

  using UStream = std::basic_stringstream<uchar>;
  using UInStream = std::basic_istringstream<uchar>;
  using UOutStream = std::basic_ostringstream<uchar>;
}

#endif
