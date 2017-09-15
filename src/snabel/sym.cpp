#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/sym.hpp"

namespace snabel {
  Sym::Sym(Pos *pos):
    pos(pos)
  { }

  static void sym_imp(Scope &scp, const Args &args) {
    push(scp, scp.exec.sym_type, get_sym(scp.exec, *get<StrRef>(args.at(0))));
  }

  static void str_imp(Scope &scp, const Args &args) {
    push(scp,
	 scp.exec.str_type,
	 std::make_shared<str>(name(get<Sym>(args.at(0)))));
  }

  void init_syms(Exec &exe) {
    exe.tok_type.supers.push_back(&exe.any_type);
    exe.tok_type.supers.push_back(&exe.ordered_type);

    exe.tok_type.fmt = [&exe](auto &v) {
      auto &t(get<Tok>(v));
      return fmt("Tok('%0' %1 %2)", t.text, t.pos.row, t.pos.col);
    };

    exe.tok_type.eq = [](auto &x, auto &y) {
      auto &xt(get<Tok>(x)), &yt(get<Tok>(y));
      return xt.pos == yt.pos && xt.text == yt.text;
    };

    exe.tok_type.lt = [](auto &x, auto &y) {
      auto &xt(get<Tok>(x)), &yt(get<Tok>(y));
      return xt.pos < yt.pos || (xt.pos == yt.pos && xt.text < yt.text);
    };

    exe.sym_type.supers.push_back(&exe.any_type);
    exe.sym_type.supers.push_back(&exe.ordered_type);
    exe.sym_type.fmt = [&exe](auto &v) { return fmt("#%0", name(get<Sym>(v))); };

    exe.sym_type.eq = [](auto &x, auto &y) {
      return get<Sym>(x) == get<Sym>(y);
    };

    exe.sym_type.lt = [](auto &x, auto &y) {
      return get<Sym>(x) < get<Sym>(y);
    };

    exe.quote_type.supers.push_back(&exe.any_type);
    exe.quote_type.supers.push_back(&exe.ordered_type);
    exe.quote_type.fmt = [&exe](auto &v) { return fmt("´%0", *get<StrRef>(v)); };
    exe.quote_type.eq = exe.str_type.eq;
    exe.quote_type.lt = exe.str_type.lt;
    
    add_func(exe, "sym", Func::Const, {ArgType(exe.str_type)}, sym_imp);
    add_func(exe, "str", Func::Const, {ArgType(exe.sym_type)}, str_imp);
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

