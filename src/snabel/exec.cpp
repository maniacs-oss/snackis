#include <iostream>
#include "snabel/compiler.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/type.hpp"

namespace snabel {
  static void zero_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.exec);
    bool res(get<int64_t>(args[0]) == 0);
    push(scp.coro, exe.bool_type, res);
  }

  static void inc_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    auto &a(args[0]);
    push(scp.coro, *a.type, get<int64_t>(a)+1);
  }

  static void dec_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    auto &a(args[0]);
    push(scp.coro, *a.type, get<int64_t>(a)-1);
  }

  static void add_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.exec);
    int64_t res(0);

    for (auto &a: args) {
      CHECK(a.type == &exe.i64_type, _);
      res += get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void sub_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.exec);
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
    Exec &exe(scp.exec);
    int64_t res(1);

    for (auto &a: args) {
      CHECK(a.type == &exe.i64_type, _);
      res *= get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void mod_i64_imp(Scope &scp, FuncImp &fn, const Args &args) {
    Exec &exe(scp.exec);
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

  static void list_pop_imp(Scope &scp, FuncImp &fn, const Args &args) {
    auto &lst_arg(args[0]);
    auto &lst(get<ListRef>(lst_arg)->elems);
    push(scp.coro, lst_arg);
    push(scp.coro, lst.back());
    lst.pop_back();
  }

  static void list_reverse_imp(Scope &scp, FuncImp &fn, const Args &args) {
    auto &in_arg(args[0]);
    auto &in(get<ListRef>(in_arg)->elems);
    ListRef out(new List());
    std::copy(in.rbegin(), in.rend(), std::back_inserter(out->elems));
    push(scp.coro, *in_arg.type, out); 
  }

  static void thread_imp(Scope &scp, FuncImp &fn, const Args &args) {
    auto &t(start_thread(scp, args[0]));
    push(scp.coro, scp.exec.thread_type, &t);
  }

  static void join_imp(Scope &scp, FuncImp &fn, const Args &args) {
    join(*get<Thread *>(args[0]), scp);
  }

  Exec::Exec():
    main_thread(threads.emplace(std::piecewise_construct,
				std::forward_as_tuple(0),
				std::forward_as_tuple(*this, 0)).first->second),
    main(main_thread.main),
    main_scope(main.scopes.front()),
    any_type(add_type(*this, "Any")),
    meta_type(add_type(*this, "Type", any_type)),
    bool_type(add_type(*this, "Bool", any_type)),
    callable_type(add_type(*this, "Callable", any_type)),
    func_type(add_type(*this, "Func", callable_type)),
    i64_type(add_type(*this, "I64", any_type)),
    label_type(add_type(*this, "Label", callable_type)),
    lambda_type(add_type(*this, "Lambda", callable_type)),
    list_type(get_list_type(*this, any_type)),
    str_type(add_type(*this, "Str", any_type)),
    thread_type(add_type(*this, "Thread", any_type)),
    undef_type(add_type(*this, "Undef")),
    void_type(add_type(*this, "Void")),
    next_gensym(1)
  {
    meta_type.fmt = [](auto &v) { return get<Type *>(v)->name; };
    meta_type.eq = [](auto &x, auto &y) { return get<Type *>(x) == get<Type *>(y); };
    
    any_type.fmt = [](auto &v) { return "n/a"; };
    any_type.eq = [](auto &x, auto &y) { return false; };

    callable_type.fmt = [](auto &v) { return "n/a"; };
    callable_type.eq = [](auto &x, auto &y) { return false; };

    undef_type.fmt = [](auto &v) { return "n/a"; };
    undef_type.eq = [](auto &x, auto &y) { return true; };
    put_env(main_scope, "'undef", Box(undef_type, undef));
    
    void_type.fmt = [](auto &v) { return "n/a"; };
    void_type.eq = [](auto &x, auto &y) { return true; };  

    bool_type.fmt = [](auto &v) { return get<bool>(v) ? "'t" : "'f"; };
    bool_type.eq = [](auto &x, auto &y) { return get<bool>(x) == get<bool>(y); };
    put_env(main_scope, "'t", Box(bool_type, true));
    put_env(main_scope, "'f", Box(bool_type, false));

    func_type.fmt = [](auto &v) { return fmt_arg(size_t(get<Func *>(v))); };
    func_type.eq = [](auto &x, auto &y) { return get<Func *>(x) == get<Func *>(y); };
    func_type.call.emplace([](auto &scp, auto &v) {
	auto &cor(scp.coro);
	auto &fn(*get<Func *>(v));
	auto imp(match(fn, cor));

	if (!imp) {
	  ERROR(Snabel, fmt("Function not applicable: %0\n%1", 
			    fn.name, curr_stack(cor)));
	  return false;
	}

	(*imp)(cor);
	return true;
      });

    i64_type.fmt = [](auto &v) { return fmt_arg(get<int64_t>(v)); };
    i64_type.eq = [](auto &x, auto &y) { return get<int64_t>(x) == get<int64_t>(y); };
    i64_type.iter = [](auto &cnd, auto &tgt) {
      Iter it(Range(0, get<int64_t>(cnd)), tgt);
      
      it.next = [](auto &_cnd, auto &scp) -> opt<Box> {
	auto &cnd(get<Range>(_cnd));
	if (cnd.beg == cnd.end) { return nullopt; }
	auto res(cnd.beg);
	cnd.beg++;
	return Box(scp.exec.i64_type, res);
      };

      return it;
    };
    
    label_type.fmt = [](auto &v) { return get<Label *>(v)->tag; };
    label_type.eq = [](auto &x, auto &y) {
      return get<Label *>(x) == get<Label *>(y);
    };
    label_type.call.emplace([](auto &scp, auto &v) {
	jump(scp.coro, *get<Label *>(v));
	return true;
      });

    lambda_type.fmt = [](auto &v) { return get<str>(v); };
    lambda_type.eq = [](auto &x, auto &y) { return get<str>(x) == get<str>(y); };
    lambda_type.call.emplace([](auto &scp, auto &v) {
	auto lbl(find_label(scp.exec, get<str>(v)));

	if (!lbl) {
	  ERROR(Snabel, "Missing lambda label");
	  return false;
	}

	call(scp, *lbl);
	return true;
      });

    str_type.fmt = [](auto &v) { return fmt("\"%0\"", get<str>(v)); };
    str_type.eq = [](auto &x, auto &y) { return get<str>(x) == get<str>(y); };

    thread_type.fmt = [](auto &v) { return fmt_arg(get<Thread *>(v)->id); };
    thread_type.eq = [](auto &x, auto &y) {
      return get<Thread *>(x) == get<Thread *>(y);
    };
 
    add_func(*this, "zero?",
	     {ArgType(i64_type)}, {ArgType(bool_type)},
	     zero_i64_imp);

    add_func(*this, "inc",
	     {ArgType(i64_type)}, {ArgType(i64_type)},
	     inc_i64_imp);

    add_func(*this, "dec",
	     {ArgType(i64_type)}, {ArgType(i64_type)},
	     dec_i64_imp);

    add_func(*this, "+",
	     {ArgType(i64_type), ArgType(i64_type)}, {ArgType(i64_type)},
	     add_i64_imp);
    add_func(*this, "-",
	     {ArgType(i64_type), ArgType(i64_type)}, {ArgType(i64_type)},
	     sub_i64_imp);
    add_func(*this, "*",
	     {ArgType(i64_type), ArgType(i64_type)}, {ArgType(i64_type)},
	     mul_i64_imp);
    add_func(*this, "%",
	     {ArgType(i64_type), ArgType(i64_type)}, {ArgType(i64_type)},
	     mod_i64_imp);

    add_func(*this, "push",
	     {ArgType(list_type), ArgType(0, 0)}, {ArgType(0)},
	     list_push_imp);
    add_func(*this, "pop",
	     {ArgType(list_type)}, {ArgType(0), ArgType(0, 0)},
	     list_pop_imp);
    add_func(*this, "reverse",
	     {ArgType(list_type)}, {ArgType(0)},
	     list_reverse_imp);

    add_func(*this, "thread",
	     {ArgType(callable_type)}, {ArgType(bool_type)},
	     thread_imp);

    add_func(*this, "join",
	     {ArgType(thread_type)}, {ArgType(any_type)},
	     join_imp);

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

    add_macro(*this, "func:", [this](auto pos, auto &in, auto &out) {
	if (in.size() < 2) {
	  ERROR(Snabel, fmt("Malformed func on row %0, col %1",
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
	  out.emplace_back(Putenv(n));
	}
      });

    add_macro(*this, "let:", [this](auto pos, auto &in, auto &out) {
	if (in.size() < 2) {
	  ERROR(Snabel, fmt("Malformed let on row %0, col %1",
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
	  out.emplace_back(Putenv(fmt("$%0", n)));
	}
      });

    add_macro(*this, "call", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Call());
      });

    add_macro(*this, "drop", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Drop(1));
      });

    add_macro(*this, "dup", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Dup());
      });

    add_macro(*this, "for", [](auto pos, auto &in, auto &out) {
	out.emplace_back(For());
      });

    add_macro(*this, "getenv", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Getenv());
      });

    add_macro(*this, "recall", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Recall());
      });

    add_macro(*this, "reset", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Reset());
      });

    add_macro(*this, "return", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Return(false));
      });

    add_macro(*this, "stash", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Stash());
      });

    add_macro(*this, "swap", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Swap());
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
    put_env(exe.main_scope, n, Box(exe.meta_type, &res));
    return res;
  }

  Type &add_type(Exec &exe, const str &n, Type &super) {
    auto &res(exe.types.emplace_front(n, super)); 
    put_env(exe.main_scope, n, Box(exe.meta_type, &res));
    return res;
  }

  Type &get_list_type(Exec &exe, Type &elt) {    
    str n(fmt("List<%0>", elt.name));
    auto fnd(find_env(exe.main_scope, n));
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
		    const ArgTypes &results,
		    FuncImp::Imp imp) {
    auto fnd(exe.funcs.find(n));

    if (fnd == exe.funcs.end()) {
      auto &fn(exe.funcs.emplace(std::piecewise_construct,
				  std::forward_as_tuple(n),
				  std::forward_as_tuple(n)).first->second);
      put_env(exe.main_scope, n, Box(exe.func_type, &fn));
      return add_imp(fn, args, results, imp);
    }
    
    return add_imp(fnd->second, args, results, imp);
  }

  Label &add_label(Exec &exe, const str &tag) {
    auto &l(exe.labels
      .emplace(std::piecewise_construct,
	       std::forward_as_tuple(tag),
	       std::forward_as_tuple(tag))
	    .first->second);
    put_env(exe.main_scope, tag, Box(exe.label_type, &l));
    return l;
  }

  void clear_labels(Exec &exe) {
    for (auto &l: exe.labels) {
      rem_env(exe.main_scope, l.first);
    }
    
    exe.labels.clear();
  }

  Label *find_label(Exec &exe, const str &tag) {
    auto fnd(exe.labels.find(tag));
    if (fnd == exe.labels.end()) { return nullptr; }
    return &fnd->second;
  }
  
  Sym gensym(Exec &exe) {
    return exe.next_gensym.fetch_add(1);
  }
  
  bool run(Exec &exe, const str &in) {
    if (!compile(exe.main, in)) { return false; }
    return run(exe.main);
  }
}
