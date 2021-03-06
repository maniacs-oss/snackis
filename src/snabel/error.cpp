#include "snabel/error.hpp"
#include "snabel/func.hpp"

namespace snabel {
  SnabelError::SnabelError(const str &msg):
    Error(str("Error: ") + msg)
  { }
  
  TestError::TestError(const str &msg):
    SnabelError(msg)
  { }

  UnknownIdError::UnknownIdError(const Sym &id):
    SnabelError(fmt("Unknown identifier: %0", name(id)))
  { }
  
  UnsafeCallError::UnsafeCallError(const Func &fn):
    SnabelError(fmt("Unsafe call not allowed: %0", name(fn.name)))
  { }

  UnsafeStackError::UnsafeStackError():
    SnabelError(fmt("Unsafe stack access"))
  { }
}
