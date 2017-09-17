#ifndef SNACKIS_IO_HPP
#define SNACKIS_IO_HPP

#include "snackis/core/opt.hpp"
#include "snackis/core/path.hpp"
#include "snackis/core/str.hpp"

namespace snackis {
  opt<str> slurp(const Path &p);
}

#endif
