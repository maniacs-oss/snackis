#ifndef SNABEL_ENV_HPP
#define SNABEL_ENV_HPP

#include <map>
#include "snabel/box.hpp"
#include "snabel/sym.hpp"

namespace snabel {
  using Env = std::map<Sym, Box>;
}

#endif
