#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/sym.hpp"

namespace snabel {
  static const str &add_sym(Exec &exe, const str &n, Sym &s) {
    auto res(exe.syms.emplace(std::piecewise_construct,
			      std::forward_as_tuple(n),
			      std::forward_as_tuple(&s)));
    if (!res.second) {
      ERROR(Snabel, fmt("Sym already exists: %0", n));
    }

    return res.first->first;
  }

  Sym::Sym(Exec &exe, const str &n):
    exec(exe), name(add_sym(exe, n, *this))
  { }

  Sym::~Sym() {
    exec.syms.erase(name);
  }
}
