#include <fstream>

#include "snackis/core/error.hpp"
#include "snackis/core/fmt.hpp"
#include "snackis/core/io.hpp"
#include "snackis/core/stream.hpp"

namespace snackis {
  opt<str> slurp(const Path &p) {
    std::ifstream f;
    f.open(p.string(), std::ifstream::in);
    
    if (f.fail()) {
      ERROR(Core, fmt("Failed opening file: %0", p.string()));
      return nullopt;
    }
    
    OutStream buf;
    buf << f.rdbuf();
    f.close();
    return buf.str();
  }
}
