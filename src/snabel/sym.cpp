#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/sym.hpp"

namespace snabel {
  Sym::Sym(Exec &exe, SymTable::iterator it):
    exec(exe),
    it(it),
    pos((it == exe.syms.begin()) ? 0 : std::prev(it)->second.lock()->pos+1)
  { }

  Sym::~Sym() {
    exec.syms.erase(it);
  }

  bool operator ==(const Sym &x, const Sym &y) {
    return x.pos == y.pos;
  }
  
  bool operator <(const Sym &x, const Sym &y) {
    return x.pos < y.pos;
  }

  static void str_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.str_type, name(*get<SymRef>(args.at(0))));
  }

  void init_syms(Exec &exe) {
    exe.sym_type.supers.push_back(&exe.any_type);
    exe.sym_type.supers.push_back(&exe.ordered_type);
    exe.sym_type.fmt = [](auto &v) { return fmt("#%0", name(*get<SymRef>(v))); };

    exe.sym_type.eq = [](auto &x, auto &y) {
      return *get<SymRef>(x) == *get<SymRef>(y);
    };

    exe.sym_type.lt = [](auto &x, auto &y) {
      return *get<SymRef>(x) < *get<SymRef>(y);
    };

    add_func(exe, "str", {ArgType(exe.sym_type)}, str_imp);
  }

  SymRef get_sym(Exec &exe, const str &n) {
    auto res(exe.syms.emplace(std::piecewise_construct,
			      std::forward_as_tuple(n),
			      std::forward_as_tuple()));
    if (res.second) {
      auto ptr(std::make_shared<Sym>(exe, res.first));
      res.first->second = ptr;
      for (auto i(res.first); i != exe.syms.end(); i++) { i->second.lock()->pos++; }
      return ptr;
    }

    return res.first->second.lock();
  }

  const str &name(const Sym &sym) { return sym.it->first; }
}

