#ifndef SNABEL_STRUCT_HPP
#define SNABEL_STRUCT_HPP

#include "snabel/box.hpp"
#include "snabel/sym.hpp"
#include "snackis/core/fmt.hpp"

namespace snabel {
  struct Exec;

  struct Struct {
    using Fields = std::map<Sym, Box>;
    Type &type;
    Fields fields;
    Struct(Type &t);
  };
  
  bool operator==(const Struct &x, const Struct &y);
  void init_structs(Exec &exe);
  Type &get_struct_type(Exec &exe, const Sym &n);
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Struct &arg);
}

#endif
