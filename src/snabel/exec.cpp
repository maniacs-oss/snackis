#include <iostream>
#include "snabel/compiler.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/type.hpp"

namespace snabel {
  static void add_i64(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(0);

    for (auto &a: args) {
      CHECK(&a.type == &exe.i64_type, _);
      res += get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void sub_i64(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(get<int64_t>(args[0]));

    if (args.size() == 1) { res = -res; }
    else {
      for (auto i=std::next(args.begin()); i != args.end(); i++) {
	CHECK(&i->type == &exe.i64_type, _);
	res -= get<int64_t>(*i);
      }
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void mul_i64(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(1);

    for (auto &a: args) {
      CHECK(&a.type == &exe.i64_type, _);
      res *= get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void mod_i64(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.coro.exec);
    int64_t res(get<int64_t>(args[0]));
    for (auto i=std::next(args.begin()); i != args.end(); i++) {
      CHECK(&i->type == &exe.i64_type, _);
      res %= get<int64_t>(*i);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  Exec::Exec():
    main(fibers.emplace(std::piecewise_construct,
			std::forward_as_tuple(null_uid),
			std::forward_as_tuple(*this, null_uid)).first->second),
    meta_type((add_type(*this, "Type"))),
    func_type((add_type(*this, "Func"))),
    i64_type((add_type(*this, "I64"))),
    lambda_type((add_type(*this, "Lambda"))),
    str_type((add_type(*this, "Str"))),
    undef_type((add_type(*this, "Undef"))),
    void_type((add_type(*this, "Void"))),
    next_sym(1)
  {
    meta_type.fmt = [](auto &v) { return get<Type *>(v)->name; };
    func_type.fmt = [](auto &v) { return fmt_arg(size_t(get<Func *>(v))); };
    i64_type.fmt = [](auto &v) { return fmt_arg(get<int64_t>(v)); };
    lambda_type.fmt = [](auto &v) { return "n/a"; };
    str_type.fmt = [](auto &v) { return fmt("\"%0\"", get<str>(v)); };
    undef_type.fmt = [](auto &v) { return "n/a"; };
    void_type.fmt = [](auto &v) { return "n/a"; };

    add_func(*this, "+", {&i64_type, &i64_type}, i64_type, add_i64);
    add_func(*this, "-", {&i64_type, &i64_type}, i64_type, sub_i64);
    add_func(*this, "*", {&i64_type, &i64_type}, i64_type, mul_i64);
    add_func(*this, "%", {&i64_type, &i64_type}, i64_type, mod_i64);

    add_macro(*this, "(", [](auto pos, auto &in, auto &out) {
	out.push_back(Op::make_group(false));
      });

    add_macro(*this, ")", [](auto pos, auto &in, auto &out) {
	out.push_back(Op::make_ungroup());
      });
    
    add_macro(*this, "begin", [](auto pos, auto &in, auto &out) {
	out.push_back(Op::make_lambda());
      });
    
    add_macro(*this, "drop", [](auto pos, auto &in, auto &out) {
	out.push_back(Op::make_drop());
      });
    
    add_macro(*this, "end", [](auto pos, auto &in, auto &out) {
	out.push_back(Op::make_unlambda());
      });
    
    add_macro(*this, "call", [](auto pos, auto &in, auto &out) {
      out.push_back(Op::make_call());
      });

    add_macro(*this, "let", [this](auto pos, auto &in, auto &out) {
	if (in.size() < 2) {
	  ERROR(Snabel, fmt("Malformed binding on row %0, col %1",
			    pos.row, pos.col));
	} else {
	  out.push_back(Op::make_backup(false));
	  const str n(in.front().text);
	  auto i(std::next(in.begin()));
	  
	  for (; i != in.end(); i++) {
	    if (i->text == ";") { break; }
	  }
	  
	  compile(*this, pos.row, TokSeq(std::next(in.begin()), i), out);
	  if (i != in.end()) { i++; }
	  in.erase(in.begin(), i);
	  out.push_back(Op::make_restore());
	  out.push_back(Op::make_let(fmt("$%0", n)));
	}
      });
    
    add_macro(*this, "reset", [](auto pos, auto &in, auto &out) {
	out.push_back(Op::make_reset());
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

  FuncImp &add_func(Exec &exe,
		    const str n,
		    const ArgTypes &args,
		    Type &rt,
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
    return exe.next_sym.fetch_add(1);
  }
}
