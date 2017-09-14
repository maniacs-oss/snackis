#ifndef SNACKIS_SNABEL_ERROR_HPP
#define SNACKIS_SNABEL_ERROR_HPP

#include "snackis/core/error.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Func;
  struct Sym;
  
  struct SnabelError: Error {
    SnabelError(const str &msg);
  };

  struct UnknownIdError: SnabelError {
    UnknownIdError(const Sym &id);
  };

  struct UnsafeCallError: SnabelError {
    UnsafeCallError(const Func &fn);
  };

  struct UnsafeStackError: SnabelError {
    UnsafeStackError();
  };
}

#endif
