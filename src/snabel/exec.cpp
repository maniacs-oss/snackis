#include <iostream>

#include "snabel/box.hpp"
#include "snabel/compiler.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/range.hpp"
#include "snabel/type.hpp"

namespace snabel {
  static void nop_imp(Scope &scp, const Args &args)
  { }

  static void isa_imp(Scope &scp, const Args &args) {
    auto &v(args[0]);
    auto &t(args[1]);
    push(scp.coro, scp.exec.bool_type, isa(v, *get<Type *>(t)));
  }

  static void type_imp(Scope &scp, const Args &args) {
    auto &v(args[0]);
    push(scp.coro, scp.exec.meta_type, v.type);
  }

  static void eq_imp(Scope &scp, const Args &args) {
    auto &x(args[0]), &y(args[1]);
    push(scp.coro, scp.exec.bool_type, x.type->eq(x, y));
  }

  static void equal_imp(Scope &scp, const Args &args) {
    auto &x(args[0]), &y(args[1]);
    push(scp.coro, scp.exec.bool_type, x.type->equal(x, y));
  }
  
  static void zero_i64_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    bool res(get<int64_t>(args[0]) == 0);
    push(scp.coro, exe.bool_type, res);
  }

  static void inc_i64_imp(Scope &scp, const Args &args) {
    auto &a(args[0]);
    push(scp.coro, *a.type, get<int64_t>(a)+1);
  }

  static void dec_i64_imp(Scope &scp, const Args &args) {
    auto &a(args[0]);
    push(scp.coro, *a.type, get<int64_t>(a)-1);
  }

  static void add_i64_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    int64_t res(0);

    for (auto &a: args) {
      CHECK(a.type == &exe.i64_type, _);
      res += get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void sub_i64_imp(Scope &scp, const Args &args) {
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

  static void mul_i64_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    int64_t res(1);

    for (auto &a: args) {
      CHECK(a.type == &exe.i64_type, _);
      res *= get<int64_t>(a);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void mod_i64_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    int64_t res(get<int64_t>(args[0]));
    for (auto i=std::next(args.begin()); i != args.end(); i++) {
      CHECK(i->type == &exe.i64_type, _);
      res %= get<int64_t>(*i);
    }
    
    push(scp.coro, exe.i64_type, res);
  }

  static void iter_imp(Scope &scp, const Args &args) {
    auto &in(args[0]);
    auto it((*in.type->iter)(in));
    push(scp.coro, *it.first, std::make_shared<Iter>(it.second));
  }

  static void iter_join_imp(Scope &scp, const Args &args) {
    auto &in(args[0]);
    auto &sep(args[1]);
    OutStream out;
    auto i((*in.type->iter)(in).second);
    bool first(true);
    
    while (true) {
      auto v(i(scp));
      if (!v) { break; }
      if (!first) { out << sep.type->fmt(sep); }
      out << v->type->fmt(*v);
      first = false;
    }
    
    push(scp.coro, scp.exec.str_type, out.str()); 
  }

  static void iter_list_imp(Scope &scp, const Args &args) {
    auto &in(args[0]);
    auto out(make_list());
    auto i((*in.type->iter)(in).second);
    
    while (true) {
      auto v(i(scp));
      if (!v) { break; }
      out->elems.push_back(*v);
    }
    
    push(scp.coro, get_list_type(scp.exec, *in.type->args[0]), out); 
  }

  static void iter_pop_imp(Scope &scp, const Args &args) {
    auto &cor(scp.coro);
    auto &it(args[0]);
    push(cor, it);
    auto v((*get<IterRef>(it))(scp));
    if (!v) { ERROR(Snabel, "Pop of emptied iterator"); }
    push(cor, v ? *v : Box(scp.exec.undef_type, undef));
  }
  
  static void list_imp(Scope &scp, const Args &args) {
    auto &elt(args[0]);
    push(scp.coro, get_list_type(scp.exec, *get<Type *>(elt)), make_list());    
  }

  static void list_push_imp(Scope &scp, const Args &args) {
    auto &lst(args[0]);
    auto &el(args[1]);
    get<ListRef>(lst)->elems.push_back(el);
    push(scp.coro, lst);    
  }

  static void list_pop_imp(Scope &scp, const Args &args) {
    auto &lst_arg(args[0]);
    auto &lst(get<ListRef>(lst_arg)->elems);
    push(scp.coro, lst_arg);
    push(scp.coro, lst.back());
    lst.pop_back();
  }

  static void list_reverse_imp(Scope &scp, const Args &args) {
    auto &in_arg(args[0]);
    auto &in(get<ListRef>(in_arg)->elems);
    ListRef out(new List());
    std::copy(in.rbegin(), in.rend(), std::back_inserter(out->elems));
    push(scp.coro, *in_arg.type, out); 
  }

  static void thread_imp(Scope &scp, const Args &args) {
    auto &t(start_thread(scp, args[0]));
    push(scp.coro, scp.exec.thread_type, &t);
  }

  static void thread_join_imp(Scope &scp, const Args &args) {
    join(*get<Thread *>(args[0]), scp);
  }

  Exec::Exec():
    main_thread(threads.emplace(std::piecewise_construct,
				std::forward_as_tuple(0),
				std::forward_as_tuple(*this, 0)).first->second),
    main(main_thread.main),
    main_scope(main.scopes.front()),
    meta_type("Type"),
    any_type(add_type(*this, "Any")),
    bool_type(add_type(*this, "Bool")),
    callable_type(add_type(*this, "Callable")),
    char_type(add_type(*this, "Char")),
    func_type(add_type(*this, "Func")),
    i64_type(add_type(*this, "I64")),
    iter_type(add_type(*this, "Iter")),
    iterable_type(add_type(*this, "Iterable")),
    label_type(add_type(*this, "Label")),
    lambda_type(add_type(*this, "Lambda")),
    list_type(add_type(*this, "List")),
    str_type(add_type(*this, "Str")),
    thread_type(add_type(*this, "Thread")),
    undef_type(add_type(*this, "Undef")),
    void_type(add_type(*this, "Void")),
    next_gensym(1)
  {    
    any_type.fmt = [](auto &v) { return "n/a"; };
    any_type.eq = [](auto &x, auto &y) { return false; };

    meta_type.supers.push_back(&any_type);
    meta_type.fmt = [](auto &v) { return get<Type *>(v)->name; };
    meta_type.eq = [](auto &x, auto &y) { return get<Type *>(x) == get<Type *>(y); };

    undef_type.fmt = [](auto &v) { return "n/a"; };
    undef_type.eq = [](auto &x, auto &y) { return true; };
    put_env(main_scope, "'undef", Box(undef_type, undef));
    
    void_type.fmt = [](auto &v) { return "n/a"; };
    void_type.eq = [](auto &x, auto &y) { return true; };  

    callable_type.fmt = [](auto &v) { return "n/a"; };
    callable_type.eq = [](auto &x, auto &y) { return false; };

    iter_type.fmt = [](auto &v) { return "n/a"; };
    iter_type.eq = [](auto &x, auto &y) { return false; };

    iterable_type.fmt = [](auto &v) { return "n/a"; };
    iterable_type.eq = [](auto &x, auto &y) { return false; };

    list_type.fmt = [](auto &v) { return "n/a"; };
    list_type.eq = [](auto &x, auto &y) { return false; };

    bool_type.supers.push_back(&any_type);
    bool_type.fmt = [](auto &v) { return get<bool>(v) ? "'t" : "'f"; };
    bool_type.eq = [](auto &x, auto &y) { return get<bool>(x) == get<bool>(y); };
    put_env(main_scope, "'t", Box(bool_type, true));
    put_env(main_scope, "'f", Box(bool_type, false));

    char_type.supers.push_back(&any_type);

    char_type.dump = [](auto &v) -> str {
      auto c(get<char>(v));
      
      switch (c) {
      case ' ':
	return "\\space";
      case '\n':
	return "\\n";
      case '\t':
	return "\\t";
      }

      return fmt("\\%0", str(1, c));
    };

    char_type.fmt = [](auto &v) { return str(1, get<char>(v)); };
    
    char_type.eq = [](auto &x, auto &y) { return get<char>(x) == get<char>(y); };
    
    func_type.supers.push_back(&any_type);
    func_type.supers.push_back(&callable_type);
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

	(*imp)(scp);
	return true;
      });

    i64_type.supers.push_back(&any_type);
    i64_type.supers.push_back(&get_iterable_type(*this, i64_type));
    i64_type.fmt = [](auto &v) { return fmt_arg(get<int64_t>(v)); };
    i64_type.eq = [](auto &x, auto &y) { return get<int64_t>(x) == get<int64_t>(y); };

    i64_type.iter = [this](auto &_cnd) {
      Range cnd(0, get<int64_t>(_cnd));
      
      return std::make_pair(&get_iter_type(*this, i64_type),
			    [cnd](auto &scp) mutable -> opt<Box> {
			      if (cnd.beg == cnd.end) { return nullopt; }
			      auto res(cnd.beg);
			      cnd.beg++;
			      return Box(scp.exec.i64_type, res);
			    });
    };
    
    label_type.supers.push_back(&any_type);
    label_type.supers.push_back(&callable_type);
    label_type.fmt = [](auto &v) { return get<Label *>(v)->tag; };

    label_type.eq = [](auto &x, auto &y) {
      return get<Label *>(x) == get<Label *>(y);
    };

    label_type.call.emplace([](auto &scp, auto &v) {
	jump(scp.coro, *get<Label *>(v));
	return true;
      });

    lambda_type.supers.push_back(&any_type);
    lambda_type.supers.push_back(&callable_type);
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

    str_type.supers.push_back(&any_type);
    str_type.supers.push_back(&get_iterable_type(*this, char_type));
    str_type.fmt = [](auto &v) { return fmt("\"%0\"", get<str>(v)); };
    str_type.eq = [](auto &x, auto &y) { return get<str>(x) == get<str>(y); };

    str_type.iter = [this](auto &cnd) mutable {
      auto s(get<str>(cnd));
      size_t pos(0);
      
      return std::make_pair(&get_iter_type(*this, char_type),
			    [s, pos](auto &scp) mutable -> opt<Box> {
			      if (pos == s.size()) { return nullopt; }
			      auto res(s[pos]);
			      pos++;
			      return Box(scp.exec.char_type, res);
			    });
    };

    thread_type.supers.push_back(&any_type);
    thread_type.fmt = [](auto &v) { return fmt_arg(get<Thread *>(v)->id); };
    thread_type.eq = [](auto &x, auto &y) {
      return get<Thread *>(x) == get<Thread *>(y);
    };
 
    add_func(*this, "nop", {}, {}, nop_imp);

    add_func(*this, "isa?",
	     {ArgType(any_type), ArgType(meta_type)}, {ArgType(bool_type)},
	     isa_imp);

    add_func(*this, "type",
	     {ArgType(any_type)}, {ArgType(0)},
	     type_imp);

    add_func(*this, "=",
	     {ArgType(any_type), ArgType(0)}, {ArgType(bool_type)},
	     eq_imp);

    add_func(*this, "==",
	     {ArgType(any_type), ArgType(0)}, {ArgType(bool_type)},
	     equal_imp);
    
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

    add_func(*this, "iter",
	     {ArgType(iterable_type)},
	     {ArgType(0, 0, [this](auto &elt) { 
		   return &get_iter_type(*this, elt); 
		 })},
	     iter_imp);

    add_func(*this, "join",
	     {ArgType(iterable_type), ArgType(any_type)}, {ArgType(str_type)},
	     iter_join_imp);

    add_func(*this, "list",
	     {ArgType(iterable_type)},
	     {ArgType(0, 0, [this](auto &elt) {
		   return &get_list_type(*this, elt);
		 })},
	     iter_list_imp);

    add_func(*this, "pop",
	     {ArgType(iter_type)}, {ArgType(0), ArgType(0, 0)},
	     iter_pop_imp);
    
    add_func(*this, "list",
	     {ArgType(meta_type)},
	     {ArgType(0, [this](auto &elt) { return &get_list_type(*this, elt); })},
	     list_imp);
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
	     thread_join_imp);

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
	out.emplace_back(Backup());
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
	  out.emplace_back(Backup());
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
	  out.emplace_back(Backup());
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
    auto &res(exe.types.emplace_back(n)); 
    put_env(exe.main_scope, n, Box(exe.meta_type, &res));
    return res;
  }

  Type &get_iter_type(Exec &exe, Type &elt) {    
    str n(fmt("Iter<%0>", elt.name));
    auto fnd(find_env(exe.main_scope, n));
    if (fnd) { return *get<Type *>(*fnd); }
    auto &t(add_type(exe, n));
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&exe.iter_type);
    t.supers.push_back(&get_iterable_type(exe, elt));    
    t.args.push_back(&elt);
    t.fmt = [&elt](auto &v) { return "n/a"; };
    t.eq = [&elt](auto &x, auto &y) { return x == y; };

    t.iter = [&exe](auto &cnd) {
      return std::make_pair(cnd.type, *get<IterRef>(cnd));
    };
    
    return t;
  }

  Type &get_iterable_type(Exec &exe, Type &elt) {    
    str n(fmt("Iterable<%0>", elt.name));
    auto fnd(find_env(exe.main_scope, n));
    if (fnd) { return *get<Type *>(*fnd); }
    auto &t(add_type(exe, n));
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&exe.iterable_type);
    t.args.push_back(&elt);
    t.fmt = [&elt](auto &v) { return "n/a"; };
    t.eq = [&elt](auto &x, auto &y) { return x == y; };
    return t;
  }

  Type &get_list_type(Exec &exe, Type &elt) {    
    str n(fmt("List<%0>", elt.name));
    auto fnd(find_env(exe.main_scope, n));
    if (fnd) { return *get<Type *>(*fnd); }
    auto &t(add_type(exe, n));
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&exe.list_type);
    t.supers.push_back(&get_iterable_type(exe, elt));
    t.args.push_back(&elt);
    
    t.dump = [&elt](auto &v) {
      auto &ls(get<ListRef>(v)->elems);

      OutStream buf;
      buf << '[';

      if (ls.size() < 4) {
	for (size_t i(0); i < ls.size(); i++) {
	  if (i > 0) { buf << ' '; }
	  auto &v(ls[i]);
	  buf << v.type->dump(v);
	};
      } else {
	buf <<
	ls.front().type->dump(ls.front()) <<
	"..." <<
	ls.back().type->dump(ls.back());
      }
      
      buf << ']';
      return buf.str();
    };

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
	"..." <<
	ls.back().type->fmt(ls.back());
      }
      
      buf << ']';
      return buf.str();
    };
    
    t.eq = [](auto &x, auto &y) { return get<ListRef>(x) == get<ListRef>(y); };

    t.equal = [](auto &x, auto &y) {
      auto &xs(get<ListRef>(x)->elems), &ys(get<ListRef>(y)->elems);
      if (xs.size() != ys.size()) { return false; }
      auto xi(xs.begin()), yi(ys.begin());

      for (; xi != xs.end() && yi != ys.end(); xi++, yi++) {
	if (xi->type != yi->type || !xi->type->equal(*xi, *yi)) { return false; }
      }

      return true;
    };

    t.iter = [&exe](auto &cnd) {
      auto lst(get<ListRef>(cnd));
      auto imp(lst->elems.begin());
      
      return std::make_pair(&get_iter_type(exe, *cnd.type->args.front()),
			    [lst, imp](auto &scp) mutable -> opt<Box> {
			      if (imp == lst->elems.end()) { return nullopt; }
			      auto res(*imp);
			      imp++;
			      return res;
			    });
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
