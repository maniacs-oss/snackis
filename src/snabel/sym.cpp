#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/sym.hpp"

namespace snabel {
  Sym::Sym(Pos *pos):
    pos(pos)
  { }

  /*  Sym::Sym(const Sym &src):
    it(src.it), pos(src.pos)
  { }
  
  const Sym &Sym::operator =(const Sym &src) {
    it = src.it;
    pos = src.pos;
    return *this;
    }*/

  bool operator ==(const Sym &x, const Sym &y) {
    return x.pos == y.pos;
  }

  bool operator <(const Sym &x, const Sym &y) {
    return *x.pos < *y.pos;
  }

  static void sym_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.sym_type, get_sym(scp.exec, get<str>(args.at(0))));
  }

  static void str_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.str_type, name(get<Sym>(args.at(0))));
  }

  void init_syms(Exec &exe) {
    exe.sym_type.supers.push_back(&exe.any_type);
    exe.sym_type.supers.push_back(&exe.ordered_type);
    exe.sym_type.fmt = [&exe](auto &v) { return fmt("#%0", name(get<Sym>(v))); };

    exe.sym_type.eq = [](auto &x, auto &y) {
      return get<Sym>(x) == get<Sym>(y);
    };

    exe.sym_type.lt = [](auto &x, auto &y) {
      return get<Sym>(x) < get<Sym>(y);
    };

    add_func(exe, "sym", {ArgType(exe.str_type)}, sym_imp);
    add_func(exe, "str", {ArgType(exe.sym_type)}, str_imp);
  }

  const Sym &get_sym(Exec &exe, const str &n) {
    auto fnd(exe.syms.find(n));
    if (fnd != exe.syms.end()) { return fnd->second; }

    auto res(exe.syms.emplace(std::piecewise_construct,
			      std::forward_as_tuple(n),
			      std::forward_as_tuple(new(Sym::Pos))));
    
    auto &s(res.first->second);
    s.it = res.first;
    *s.pos = (s.it == exe.syms.begin()) ? 0 : (*std::prev(s.it)->second.pos)+1;
    for (auto i(std::next(s.it)); i != exe.syms.end(); i++) { (*i->second.pos)++; }
    return s;
  }

  const str &name(const Sym &sym) { return sym.it->first; }
}

