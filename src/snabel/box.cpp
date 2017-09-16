#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/pair.hpp"

namespace snabel {
  Box::Box(Scope &scp, Type &t, const Val &v):
    type(&t), val(v), safe_level(scp.safe_level)
  { }

  Box::Box(Scope &scp, Type &t):
    type(&get_opt_type(scp.exec, t)), safe_level(scp.safe_level)
  { }

  bool operator ==(const Box &x, const Box &y) {
    return x.type == y.type && x.type->eq(x, y);
  }
  
  bool operator !=(const Box &x, const Box &y) {
    return x.type != y.type || !x.type->eq(x, y);
  }

  bool operator <(const Box &x, const Box &y) {
    if (!x.type->lt) {
      ERROR(Snabel, fmt("Missing lt implementation for value: %0", x));
      return false;
    }
    
    return x.type == y.type && x.type->lt(x, y);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Box &arg) { return arg.type->dump(arg); }
}
