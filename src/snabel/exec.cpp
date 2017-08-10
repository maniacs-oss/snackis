#include <iostream>
#include "snabel/compiler.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/type.hpp"

namespace snabel {
  static void add_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(0);

    for (auto &a: args) {
      CHECK(a.type == &exe.i64_type, _);
      res += get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void sub_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(get<int64_t>(args[0]));

    if (args.size() == 1) { res = -res; }
    else {
      for (auto i=std::next(args.begin()); i != args.end(); i++) {
	CHECK(i->type == &exe.i64_type, _);
	res -= get<int64_t>(*i);
      }
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void mul_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(1);

    for (auto &a: args) {
      CHECK(a.type == &exe.i64_type, _);
      res *= get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void mod_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(get<int64_t>(args[0]));
    for (auto i=std::next(args.begin()); i != args.end(); i++) {
      CHECK(i->type == &exe.i64_type, _);
      res %= get<int64_t>(*i);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void list_push_imp(Scope &scp, FuncImp &fn, const Args &args) {
    auto &lst(args[0]);
    auto &el(args[1]);
    get<ListRef>(lst)->elems.push_back(el);
    push(scp.coro, lst);    
  }

  static void list_reverse_imp(Scope &scp, FuncImp &fn, const Args &args) {
    auto &in_arg(args[0]);
    auto &in(get<ListRef>(in_arg)->elems);
    ListRef out(new List());
    std::copy(in.rbegin(), in.rend(), std::back_inserter(out->elems));
    push(scp.coro, *in_arg.type, out); 
  }

  Exec::Exec():
    main(fibers.emplace(std::piecewise_construct,
			std::forward_as_tuple(0),
			std::forward_as_tuple(*this, 0)).first->second),
    any_type(add_type(*this, "Any")),
    meta_type(add_type(*this, "Type", any_type)),
    bool_type(add_type(*this, "Bool", any_type)),
    func_type(add_type(*this, "Func", any_type)),
    i64_type(add_type(*this, "I64", any_type)),
    lambda_type(add_type(*this, "Lambda", any_type)),
    list_type(get_list_type(*this, any_type)),
    str_type(add_type(*this, "Str", any_type)),
    undef_type(add_type(*this, "Undef")),
    void_type(add_type(*this, "Void")),
    next_sym(1)
  {
    meta_type.fmt = [](auto &v) { return get<Type *>(v)->name; };
    meta_type.eq = [](auto &x, auto &y) { return get<Type *>(x) == get<Type *>(y); };
    any_type.fmt = [](auto &v) { return "n/a"; };
    any_type.eq = [](auto &x, auto &y) { return false; };
    bool_type.fmt = [](auto &v) { return get<bool>(v) ? "'t" : "'f"; };
    bool_type.eq = [](auto &x, auto &y) { return get<bool>(x) == get<bool>(y); };
    put_env(main.scopes.front(), "'t", Box(bool_type, true));
    put_env(main.scopes.front(), "'f", Box(bool_type, false));

    func_type.fmt = [](auto &v) { return fmt_arg(size_t(get<Func *>(v))); };
    func_type.eq = [](auto &x, auto &y) { return get<Func *>(x) == get<Func *>(y); };
    i64_type.fmt = [](auto &v) { return fmt_arg(get<int64_t>(v)); };
    i64_type.eq = [](auto &x, auto &y) { return get<int64_t>(x) == get<int64_t>(y); };
    lambda_type.fmt = [](auto &v) { return get<str>(v); };
    lambda_type.eq = [](auto &x, auto &y) { return get<str>(x) == get<str>(y); };
    str_type.fmt = [](auto &v) { return fmt("\"%0\"", get<str>(v)); };
    str_type.eq = [](auto &x, auto &y) { return get<str>(x) == get<str>(y); };
    undef_type.fmt = [](auto &v) { return "n/a"; };
    undef_type.eq = [](auto &x, auto &y) { return true; };
    void_type.fmt = [](auto &v) { return "n/a"; };
    void_type.eq = [](auto &x, auto &y) { return true; };  
 
    add_func(*this, "+",
	     {ArgType(i64_type), ArgType(i64_type)}, ArgType(i64_type),
	     add_i64_imp);
    add_func(*this, "-",
	     {ArgType(i64_type), ArgType(i64_type)}, ArgType(i64_type),
	     sub_i64_imp);
    add_func(*this, "*",
	     {ArgType(i64_type), ArgType(i64_type)}, ArgType(i64_type),
	     mul_i64_imp);
    add_func(*this, "%",
	     {ArgType(i64_type), ArgType(i64_type)}, ArgType(i64_type),
	     mod_i64_imp);

    add_func(*this, "push",
	     {ArgType(list_type), ArgType(0, 0)}, ArgType(0),
	     list_push_imp);
    add_func(*this, "reverse",
	     {ArgType(list_type)}, ArgType(0),
	     list_reverse_imp);

    add_macro(*this, "{", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Lambda());
      });

    add_macro(*this, "}", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Unlambda());
      });

    add_macro(*this, "(", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Group(false));
      });

    add_macro(*this, ")", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Ungroup());
      });

    add_macro(*this, "[", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Backup(false));
      });

    add_macro(*this, "]", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Stash());	
	out.emplace_back(Restore());
      });

    add_macro(*this, "begin", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Lambda());
      });

    add_macro(*this, "call", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Call());
      });

    add_macro(*this, "drop", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Drop(1));
      });
    
    add_macro(*this, "end", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Unlambda());
      });

    add_macro(*this, "let:", [this](auto pos, auto &in, auto &out) {
	if (in.size() < 2) {
	  ERROR(Snabel, fmt("Malformed binding on row %0, col %1",
			    pos.row, pos.col));
	} else {
	  out.emplace_back(Backup(false));
	  const str n(in.front().text);
	  auto i(std::next(in.begin()));
	  
	  for (; i != in.end(); i++) {
	    if (i->text == ";") { break; }
	  }
	  
	  compile(*this, pos.row, TokSeq(std::next(in.begin()), i), out);
	  if (i != in.end()) { i++; }
	  in.erase(in.begin(), i);
	  out.emplace_back(Restore());
	  out.emplace_back(Let(fmt("$%0", n)));
	}
      });
    
    add_macro(*this, "reset", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Reset());
      });

    add_macro(*this, "stash", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Stash());
      });

    add_macro(*this, "when", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Branch());
      });
  }

  Macro &add_macro(Exec &exe, const str &n, Macro::Imp imp) {
    return exe.macros.emplace(std::piecewise_construct,
			      std::forward_as_tuple(n),
			      std::forward_as_tuple(n, imp)).first->second; 
  }
  
  Type &add_type(Exec &exe, const str &n) {
    auto &res(exe.types.emplace_front(n)); 
    put_env(exe.main.scopes.front(), n, Box(exe.meta_type, &res));
    return res;
  }

  Type &add_type(Exec &exe, const str &n, Type &super) {
    auto &res(exe.types.emplace_front(n, super)); 
    put_env(exe.main.scopes.front(), n, Box(exe.meta_type, &res));
    return res;
  }

  Type &get_list_type(Exec &exe, Type &elt) {    
    str n(fmt("List<%0>", elt.name));
    auto fnd(find_env(exe.main.scopes.front(), n));
    if (fnd) { return *get<Type *>(*fnd); }
    auto &t(add_type(exe, n, exe.list_type));
    t.args.push_back(&elt);
    
    t.fmt = [&elt](auto &v) {
      auto &ls(get<ListRef>(v)->elems);

      OutStream buf;
      buf << '[';

      if (ls.size() < 4) {
	for (size_t i(0); i < ls.size(); i++) {
	  if (i > 0) { buf << ' '; }
	  auto &v(ls[i]);
	  buf << v.type->fmt(v);
	};
      } else {
	buf <<
	ls.front().type->fmt(ls.front()) <<
	" : " <<
	ls.back().type->fmt(ls.back());
      }
      
      buf << ']';
      return buf.str();
    };
    
    t.eq = [&elt](auto &_x, auto &_y) {
      auto &x(get<ListRef>(_x)), &y(get<ListRef>(_y));
      
      if (x->elems.size() != y->elems.size()) { return false; }
      
      for (auto i = x->elems.begin(), j = y->elems.begin();
	   i != x->elems.end();
	   i++, j++) {
	if (!elt.eq(*i, *j)) { return false; }
      }
      
      return true;
    };
    
    return t;
  }

  FuncImp &add_func(Exec &exe,
		    const str n,
		    const ArgTypes &args,
		    const ArgType &rt,
		    FuncImp::Imp imp) {
    auto fnd(exe.funcs.find(n));

    if (fnd == exe.funcs.end()) {
      auto &fn(exe.funcs.emplace(std::piecewise_construct,
				  std::forward_as_tuple(n),
				  std::forward_as_tuple(n)).first->second);
      put_env(exe.main.scopes.front(), n, Box(exe.func_type, &fn));
      return add_imp(fn, args, rt, imp);
    }
    
    return add_imp(fnd->second, args, rt, imp);
  }
  
  Sym gensym(Exec &exe) {
    Sym s(exe.next_sym);
    exe.next_sym++;
    return s;
  }

  bool run(Exec &exe, const str &in) {
    if (!compile(exe.main, in)) { return false; }
    return run(exe.main);
  }
}
