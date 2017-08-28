#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include "snabel/box.hpp"
#include "snabel/compiler.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"
#include "snabel/iter.hpp"
#include "snabel/iters.hpp"
#include "snabel/list.hpp"
#include "snabel/opt.hpp"
#include "snabel/pair.hpp"
#include "snabel/proc.hpp"
#include "snabel/range.hpp"
#include "snabel/str.hpp"
#include "snabel/table.hpp"
#include "snabel/type.hpp"

namespace snabel {
  static void is_imp(Scope &scp, const Args &args) {
    auto &v(args.at(0));
    auto &t(args.at(1));
    push(scp.thread, scp.exec.bool_type, isa(scp.thread, v, *get<Type *>(t)));
  }

  static void type_imp(Scope &scp, const Args &args) {
    auto &v(args.at(0));
    push(scp.thread, get_meta_type(scp.exec, *v.type), v.type);
  }

  static void eq_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp.thread, scp.exec.bool_type, x.type->eq(x, y));
  }

  static void equal_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp.thread, scp.exec.bool_type, x.type->equal(x, y));
  }

  static void lt_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp.thread, scp.exec.bool_type, x.type->lt(x, y));
  }

  static void gt_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp.thread, scp.exec.bool_type, x.type->gt(x, y));
  }

  static void when_imp(Scope &scp, const Args &args) {
    auto &cnd(args.at(0)), &tgt(args.at(1));
    if (get<bool>(cnd)) { (*tgt.type->call)(scp, tgt, false); }
  }

  static void unless_imp(Scope &scp, const Args &args) {
    auto &cnd(args.at(0)), &tgt(args.at(1));
    if (!get<bool>(cnd)) { (*tgt.type->call)(scp, tgt, false); }
  }

  static void zero_i64_imp(Scope &scp, const Args &args) {
    bool res(get<int64_t>(args.at(0)) == 0);
    push(scp.thread, scp.exec.bool_type, res);
  }

  static void pos_i64_imp(Scope &scp, const Args &args) {
    bool res(get<int64_t>(args.at(0)) > 0);
    push(scp.thread, scp.exec.bool_type, res);
  }

  static void inc_i64_imp(Scope &scp, const Args &args) {
    auto &in(get<int64_t>(args.at(0)));
    push(scp.thread, scp.exec.i64_type, in+1);
  }

  static void dec_i64_imp(Scope &scp, const Args &args) {
    auto &in(get<int64_t>(args.at(0)));
    push(scp.thread, scp.exec.i64_type, in-1);
  }

  static void add_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp.thread, scp.exec.i64_type, x+y);
  }

  static void sub_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp.thread, scp.exec.i64_type, x-y);
  }

  static void mul_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp.thread, scp.exec.i64_type, x*y);
  }

  static void div_i64_imp(Scope &scp, const Args &args) {
    auto &num(get<int64_t>(args.at(0)));
    auto &div(get<int64_t>(args.at(1)));
    bool neg = (num < 0 && div > 0) || (div < 0 && num >= 0);
    push(scp.thread, scp.exec.rat_type, Rat(abs(num), abs(div), neg));
  }

  static void mod_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp.thread, scp.exec.i64_type, x%y);
  }

  static void i64_str_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.str_type, std::to_string(get<int64_t>(args.at(0))));
  }

  static void trunc_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.i64_type, trunc(get<Rat>(args.at(0))));
  }

  static void frac_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.rat_type, frac(get<Rat>(args.at(0))));
  }

  static void add_rat_imp(Scope &scp, const Args &args) {
    auto &x(get<Rat>(args.at(0)));
    auto &y(get<Rat>(args.at(1)));
    push(scp.thread, scp.exec.rat_type, x+y);
  }

  static void sub_rat_imp(Scope &scp, const Args &args) {
    auto &x(get<Rat>(args.at(0)));
    auto &y(get<Rat>(args.at(1)));
    push(scp.thread, scp.exec.rat_type, x-y);
  }

  static void mul_rat_imp(Scope &scp, const Args &args) {
    auto &x(get<Rat>(args.at(0)));
    auto &y(get<Rat>(args.at(1)));
    push(scp.thread, scp.exec.rat_type, x*y);
  }

  static void div_rat_imp(Scope &scp, const Args &args) {
    auto &x(get<Rat>(args.at(0)));
    auto &y(get<Rat>(args.at(1)));
    push(scp.thread, scp.exec.rat_type, x/y);
  }

  static void bin_zero_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.bool_type, get<BinRef>(in)->empty());
  }

  static void bin_pos_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.bool_type, !get<BinRef>(in)->empty());
  }

  static void bin_len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.i64_type, (int64_t)get<BinRef>(in)->size());
  } 

  static void bin_str_imp(Scope &scp, const Args &args) {
    auto &in(*get<BinRef>(args.at(0)));
    push(scp.thread, scp.exec.str_type, str(in.begin(), in.end()));
  }

  static void bin_ustr_imp(Scope &scp, const Args &args) {
    auto &in(*get<BinRef>(args.at(0)));
    push(scp.thread, scp.exec.ustr_type, uconv.from_bytes(str(in.begin(), in.end())));
  }

  static void bin_append_imp(Scope &scp, const Args &args) {
    auto &out(args.at(0));
    auto &tgt(*get<BinRef>(out));
    auto &src(*get<BinRef>(args.at(1)));
    std::copy(src.begin(), src.end(), std::back_inserter(tgt));
    push(scp.thread, out);
  }

  static void str_len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.i64_type, (int64_t)get<str>(in).size());
  }

  static void ustr_len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.i64_type, (int64_t)get<ustr>(in).size());
  }

  static void str_bytes_imp(Scope &scp, const Args &args) {
    auto &in(get<str>(args.at(0)));
    push(scp.thread, scp.exec.bin_type, std::make_shared<Bin>(in.begin(), in.end()));
  }

  static void str_ustr_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.ustr_type, uconv.from_bytes(get<str>(args.at(0))));
  }

  static void ustr_bytes_imp(Scope &scp, const Args &args) {
    auto in(uconv.to_bytes(get<ustr>(args.at(0))));
    push(scp.thread, scp.exec.bin_type, std::make_shared<Bin>(in.begin(), in.end()));
  }

  static void ustr_str_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.ustr_type, uconv.to_bytes(get<ustr>(args.at(0))));
  }

  static void iter_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    push(scp.thread, it->type, it);
  }

  static void iterable_str_imp(Scope &scp, const Args &args) {    
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    str out;
    
    while (true) {
      auto c(it->next(scp));
      if (!c) { break; }
      out.push_back(get<char>(*c));
    }
    
    push(scp.thread, scp.exec.str_type, out); 
  }

  static void iterable_join_str_imp(Scope &scp, const Args &args) {    
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    auto &sep(args.at(1));
    OutStream out;
    bool first(true);
    
    while (true) {
      auto v(it->next(scp));
      if (!v) { break; }
      if (!first) { out << sep.type->fmt(sep); }
      out << v->type->fmt(*v);
      first = false;
    }
    
    push(scp.thread, scp.exec.str_type, out.str()); 
  }
  
  static void iterable_filter_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0)), &tgt(args.at(1));
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 IterRef(new FilterIter(exe, it, elt, tgt)));
  }

  static void iterable_map_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0)), &tgt(args.at(1));
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 IterRef(new MapIter(exe, it, tgt)));
  }
  
  static void bytes_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.bin_type,
	 std::make_shared<Bin>(get<int64_t>(args.at(0))));
  }

  static void uid_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.uid_type, uid(scp.exec)); 
  }
  
  static void rfile_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.rfile_type,
	 std::make_shared<File>(get<Path>(args.at(0)), O_RDONLY));
  }

  static void rwfile_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.rwfile_type,
	 std::make_shared<File>(get<Path>(args.at(0)), O_RDWR | O_CREAT | O_TRUNC));
  }

  static void read_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    auto &elt(get_opt_type(exe, exe.bin_type));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 IterRef(new ReadIter(exe, elt, args.at(0))));
  }

  static void write_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    auto &in(args.at(0));
    auto &out(args.at(1));
    push(scp.thread, out);
    push(scp.thread,
	 get_iter_type(exe, exe.i64_type),
	 IterRef(new WriteIter(exe, (*in.type->iter)(in), out)));
  }

  static void iterable_lines_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, get_opt_type(exe, exe.str_type)),
	 IterRef(new SplitIter(exe, (*in.type->iter)(in), [](auto &c) {
	       return c == '\r' || c == '\n';
	     })));
  }

  static void iterable_words_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, get_opt_type(exe, exe.str_type)),
	 IterRef(new SplitIter(exe, (*in.type->iter)(in), [](auto &c) {
	       return !isalpha(c);
	     })));
  }

  static void random_imp(Scope &scp, const Args &args) {
    push(scp.thread,
	 scp.exec.random_type,
	 std::make_shared<Random>(0, get<int64_t>(args.at(0))));
  }

  static void random_pop_imp(Scope &scp, const Args &args) {
    auto &r(args.at(0));
    push(scp.thread, r);
    push(scp.thread,
	 scp.exec.i64_type,
	 (*get<RandomRef>(r))(scp.thread.random));
  }

  static void proc_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.proc_type,
	 std::make_shared<Proc>(get<CoroRef>(args.at(0))));
  }

  static void proc_run_imp(Scope &scp, const Args &args) {
    auto p(get<ProcRef>(args.at(0)));
    while (call(p, scp, true));
  }

  static void thread_imp(Scope &scp, const Args &args) {
    auto &t(start_thread(scp, args.at(0)));
    push(scp.thread, scp.exec.thread_type, &t);
  }

  static void thread_join_imp(Scope &scp, const Args &args) {
    join(*get<Thread *>(args.at(0)), scp);
  }

  Exec::Exec():
    main(threads.emplace(std::piecewise_construct,
				std::forward_as_tuple(0),
				std::forward_as_tuple(*this, 0)).first->second),
    main_scope(main.scopes.at(0)),
    meta_type("Type<Any>"),
    any_type(add_type(*this, "Any")),
    bin_type(add_type(*this, "Bin")),
    bool_type(add_type(*this, "Bool")),
    byte_type(add_type(*this, "Byte")),
    callable_type(add_type(*this, "Callable")),
    char_type(add_type(*this, "Char")),
    coro_type(add_type(*this, "Coro")),
    drop_type(add_type(*this, "Drop")),
    file_type(add_type(*this, "File")),    
    func_type(add_type(*this, "Func")),
    i64_type(add_type(*this, "I64")),
    iter_type(add_type(*this, "Iter")),
    iterable_type(add_type(*this, "Iterable")),
    label_type(add_type(*this, "Label")),
    lambda_type(add_type(*this, "Lambda")),
    list_type(add_type(*this, "List")),
    nop_type(add_type(*this, "Nop")),
    opt_type(add_type(*this, "Opt")),
    ordered_type(add_type(*this, "Ordered")),
    pair_type(add_type(*this, "Pair")),
    path_type(add_type(*this, "Path")),
    proc_type(add_type(*this, "Proc")),    
    readable_type(add_type(*this, "Readable")),
    rfile_type(add_type(*this, "RFile")),
    random_type(add_type(*this, "Random")),
    rat_type(add_type(*this, "Rat")),
    rwfile_type(add_type(*this, "RWFile")),    
    str_type(add_type(*this, "Str")),
    table_type(add_type(*this, "Table")),
    thread_type(add_type(*this, "Thread")),
    uchar_type(add_type(*this, "UChar")),
    uid_type(add_type(*this, "Uid")),
    ustr_type(add_type(*this, "UStr")),
    void_type(add_type(*this, "Void")),
    writeable_type(add_type(*this, "Writeable")),
    return_target {
    &add_label(*this, "_return", true), &add_label(*this, "_return1", true),
      &add_label(*this, "_return2", true), &add_label(*this, "_return3", true),
      &add_label(*this, "_return4", true), &add_label(*this, "_return5", true),
      &add_label(*this, "_return6", true), &add_label(*this, "_return7", true),
      &add_label(*this, "_return8", true), &add_label(*this, "_return9", true)},
    recall_target {
    &add_label(*this, "_recall", true), &add_label(*this, "_recall1", true),
      &add_label(*this, "_recall2", true), &add_label(*this, "_recall3", true),
      &add_label(*this, "_recall4", true), &add_label(*this, "_recall5", true),
      &add_label(*this, "_recall6", true), &add_label(*this, "_recall7", true),
      &add_label(*this, "_recall8", true), &add_label(*this, "_recall9", true)},
    yield_target {
    &add_label(*this, "_yield", true), &add_label(*this, "_yield1", true),
      &add_label(*this, "_yield2", true), &add_label(*this, "_yield3", true),
      &add_label(*this, "_yield4", true), &add_label(*this, "_yield5", true),
      &add_label(*this, "_yield6", true), &add_label(*this, "_yield7", true),
      &add_label(*this, "_yield8", true), &add_label(*this, "_yield9", true)},
    break_target {
      &add_label(*this, "_break", true), &add_label(*this, "_break1", true),
	&add_label(*this, "_break2", true), &add_label(*this, "_break3", true),
	&add_label(*this, "_break4", true), &add_label(*this, "_break5", true),
	&add_label(*this, "_break6", true), &add_label(*this, "_break7", true),
	&add_label(*this, "_break8", true), &add_label(*this, "_break9", true)},
    next_uid(1)
  {    
    any_type.fmt = [](auto &v) { return "Any"; };
    any_type.eq = [](auto &x, auto &y) { return false; };

    meta_type.supers.push_back(&any_type);
    meta_type.args.push_back(&any_type);
    meta_type.fmt = [](auto &v) { return fmt("%0!", get<Type *>(v)->name); };
    meta_type.eq = [](auto &x, auto &y) { return get<Type *>(x) == get<Type *>(y); };

    void_type.fmt = [](auto &v) { return "Void"; };
    void_type.eq = [](auto &x, auto &y) { return true; };  
    
    ordered_type.supers.push_back(&any_type);

    callable_type.supers.push_back(&any_type);
    callable_type.args.push_back(&any_type);

    nop_type.args.push_back(&callable_type);
    nop_type.fmt = [](auto &v) { return "Nop"; };
    nop_type.eq = [](auto &x, auto &y) { return true; };  
    nop_type.call.emplace([](auto &scp, auto &v, bool now) { return true; });
    put_env(main_scope, "#nop", Box(nop_type, n_a));    
    
    drop_type.args.push_back(&callable_type);
    drop_type.fmt = [](auto &v) { return "Drop"; };
    drop_type.eq = [](auto &x, auto &y) { return true; };  

    drop_type.call.emplace([](auto &scp, auto &v, bool now) {
	return try_pop(scp.thread) ? true : false;
      });
    
    iter_type.supers.push_back(&any_type);
    iter_type.args.push_back(&any_type);
    iter_type.fmt = [](auto &v) { return fmt("Iter<%0>", v.type->args.at(0)->name); };
    iter_type.eq = [](auto &x, auto &y) {
      return get<IterRef>(x) == get<IterRef>(y);
    };
    iter_type.iter = [](auto &in) { return get<IterRef>(in); };

    iterable_type.supers.push_back(&any_type);
    iterable_type.args.push_back(&any_type);
    iterable_type.fmt = [](auto &v) {
      return fmt("Iterable<%0>", v.type->args.at(0)->name);
    };
    iterable_type.eq = [](auto &x, auto &y) { return false; };

    readable_type.supers.push_back(&any_type);
    writeable_type.supers.push_back(&any_type);
    
    path_type.supers.push_back(&any_type);
    path_type.fmt = [](auto &v) { return get<Path>(v).string(); };
    path_type.eq = [](auto &x, auto &y) { return get<Path>(x) == get<Path>(y); };

    bin_type.supers.push_back(&any_type);
    bin_type.supers.push_back(&get_iterable_type(*this, byte_type));
    bin_type.fmt = [](auto &v) { return fmt("Bin(%0)", get<BinRef>(v)->size()); };
    bin_type.eq = [](auto &x, auto &y) { return get<BinRef>(x) == get<BinRef>(y); };
    bin_type.equal = [](auto &x, auto &y) {
      return *get<BinRef>(x) == *get<BinRef>(y);
    };
    bin_type.iter = [this](auto &in) {
      return IterRef(new BinIter(*this, get<BinRef>(in)));
    };

    byte_type.supers.push_back(&any_type);
    byte_type.supers.push_back(&ordered_type);
    byte_type.fmt = [](auto &v) { return fmt_arg(get<Byte>(v)); };
    byte_type.eq = [](auto &x, auto &y) { return get<Byte>(x) == get<Byte>(y); };
    byte_type.lt = [](auto &x, auto &y) { return get<Byte>(x) < get<Byte>(y); };

    bool_type.supers.push_back(&any_type);
    bool_type.fmt = [](auto &v) { return get<bool>(v) ? "#t" : "#f"; };
    bool_type.eq = [](auto &x, auto &y) { return get<bool>(x) == get<bool>(y); };
    put_env(main_scope, "#t", Box(bool_type, true));
    put_env(main_scope, "#f", Box(bool_type, false));

    char_type.supers.push_back(&any_type);
    char_type.supers.push_back(&ordered_type);

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

    char_type.fmt = [](auto &v) -> str { return str(1, get<char>(v)); };
    char_type.eq = [](auto &x, auto &y) { return get<char>(x) == get<char>(y); };
    char_type.lt = [](auto &x, auto &y) { return get<char>(x) < get<char>(y); };
    
    uchar_type.supers.push_back(&any_type);
    uchar_type.supers.push_back(&ordered_type);
    
    uchar_type.dump = [](auto &v) -> str {
      auto c(get<uchar>(v));
      
      switch (c) {
      case u' ':
	return "u\\space";
      case u'\n':
	return "u\\n";
      case u'\t':
	return "u\\t";
      }

      return fmt("u\\%0", uconv.to_bytes(ustr(1, c)));
    };

    uchar_type.fmt = [](auto &v) -> str {
      return uconv.to_bytes(ustr(1, get<uchar>(v)));
    };
    
    uchar_type.eq = [](auto &x, auto &y) { return get<uchar>(x) == get<uchar>(y); };
    uchar_type.lt = [](auto &x, auto &y) { return get<uchar>(x) < get<uchar>(y); };

    proc_type.supers.push_back(&any_type);
    proc_type.supers.push_back(&callable_type);
    proc_type.fmt = [](auto &v) { return fmt("Proc(%0)", get<ProcRef>(v)->id); };
    proc_type.eq = [](auto &x, auto &y) {
      return get<ProcRef>(x) == get<ProcRef>(y);
    };

    proc_type.call.emplace([](auto &scp, auto &v, bool now) {
	call(get<ProcRef>(v), scp, now);
	return true;
      });

    file_type.supers.push_back(&any_type);
    file_type.fmt = [](auto &v) {
      auto &f(*get<FileRef>(v));
      return fmt("File(%0)", f.fd);
    };
    
    file_type.eq = [](auto &x, auto &y) {
      return get<FileRef>(x) == get<FileRef>(y);
    };

    rfile_type.supers.push_back(&file_type);
    rfile_type.supers.push_back(&readable_type);
    rfile_type.fmt = [](auto &v) { return fmt("RFile(%0)", get<FileRef>(v)->fd); };
    rfile_type.eq = file_type.eq;
    rfile_type.read = [](auto &in, auto &out) {
      auto &f(*get<FileRef>(in));
      auto res(read(f.fd, &out[0], out.size()));
      if (!res) { return READ_EOF; }

      if (res == -1) {
	if (errno == EAGAIN) { return READ_AGAIN; }
	ERROR(Snabel, fmt("Failed reading from file: %0", errno));
	return READ_ERROR;
      }

      out.resize(res);
      return READ_OK;
    };

    rwfile_type.supers.push_back(&file_type);
    rwfile_type.supers.push_back(&writeable_type);
    rwfile_type.fmt = [](auto &v) { return fmt("RWFile(%0)", get<FileRef>(v)->fd); };
    rwfile_type.eq = file_type.eq;
    rwfile_type.read = rfile_type.read;
    rwfile_type.write = [](auto &out, auto data, auto len) {
      auto &f(*get<FileRef>(out));
      int res(write(f.fd, data, len));

      if (res == -1) {
	if (errno == EAGAIN) { return 0; }
	ERROR(Snabel, fmt("Failed writing to file %0: %1", f.fd, errno));
      }
      
      return res;
    };
    
    func_type.supers.push_back(&any_type);
    func_type.supers.push_back(&callable_type);
    func_type.fmt = [](auto &v) { return fmt("Func(%0)", get<Func *>(v)->name); };
    func_type.eq = [](auto &x, auto &y) { return get<Func *>(x) == get<Func *>(y); };
    
    func_type.call.emplace([](auto &scp, auto &v, bool now) {
	auto &thd(scp.thread);
	auto &fn(*get<Func *>(v));
	auto m(match(fn, thd));

	if (!m) {
	  ERROR(Snabel, fmt("Function not applicable: %0\n%1", 
			    fn.name, curr_stack(thd)));
	  return false;
	}

	(*m->first)(scp, m->second);
	return true;
      });

    i64_type.supers.push_back(&any_type);
    i64_type.supers.push_back(&ordered_type);
    i64_type.supers.push_back(&get_iterable_type(*this, i64_type));
    i64_type.fmt = [](auto &v) { return fmt_arg(get<int64_t>(v)); };
    i64_type.eq = [](auto &x, auto &y) { return get<int64_t>(x) == get<int64_t>(y); };
    i64_type.lt = [](auto &x, auto &y) { return get<int64_t>(x) < get<int64_t>(y); };
    i64_type.iter = [this](auto &in) {
      return IterRef(new RangeIter(*this, Range(0, get<int64_t>(in))));
    };
    
    label_type.supers.push_back(&any_type);
    label_type.supers.push_back(&callable_type);
    label_type.fmt = [](auto &v) {
      auto &l(*get<Label *>(v));
      return fmt("%0:%1", l.tag, l.pc);
    };

    label_type.eq = [](auto &x, auto &y) {
      return get<Label *>(x) == get<Label *>(y);
    };

    label_type.call.emplace([](auto &scp, auto &v, bool now) {
	auto &thd(scp.thread);
	auto break_pc(thd.pc);
	jump(scp, *get<Label *>(v));
	if (!now) { return true; }
	return run(thd, break_pc);
      });

    lambda_type.supers.push_back(&any_type);
    lambda_type.supers.push_back(&callable_type);

    lambda_type.fmt = [](auto &v) {
      auto &l(*get<Label *>(v));
      return fmt("Lambda(%0:%1)", l.tag, l.pc);
    };
    
    lambda_type.eq = [](auto &x, auto &y) {
      return get<Label *>(x) == get<Label *>(y);
    };

    lambda_type.call.emplace([](auto &scp, auto &v, bool now) {
	call(scp, *get<Label *>(v), now);
	return true;
      });

    coro_type.supers.push_back(&any_type);
    coro_type.supers.push_back(&callable_type);

    coro_type.fmt = [](auto &v) {
      auto &c(*get<CoroRef>(v));
      return fmt("Coro(%0:%1)", c.target.tag, c.target.pc);
    };
    
    coro_type.eq = [](auto &x, auto &y) {
      return get<CoroRef>(x) == get<CoroRef>(y);
    };

    coro_type.call.emplace([](auto &scp, auto &v, bool now) {
	call(get<CoroRef>(v), scp, now);
	return true;
      });
    
    str_type.supers.push_back(&any_type);
    str_type.supers.push_back(&ordered_type);
    str_type.supers.push_back(&get_iterable_type(*this, char_type));
    str_type.fmt = [](auto &v) { return get<str>(v); };
    str_type.dump = [](auto &v) { return fmt("'%0'", get<str>(v)); };
    str_type.eq = [](auto &x, auto &y) { return get<str>(x) == get<str>(y); };
    str_type.lt = [](auto &x, auto &y) { return get<str>(x) < get<str>(y); };

    str_type.iter = [this](auto &in) {
      return IterRef(new StrIter(*this, get<str>(in)));
    };

    ustr_type.supers.push_back(&any_type);
    ustr_type.supers.push_back(&ordered_type);
    ustr_type.supers.push_back(&get_iterable_type(*this, uchar_type));
    ustr_type.fmt = [](auto &v) {
      return fmt("u'%0'", uconv.to_bytes(get<ustr>(v)));
    };
    ustr_type.eq = [](auto &x, auto &y) { return get<ustr>(x) == get<ustr>(y); };
    ustr_type.lt = [](auto &x, auto &y) { return get<ustr>(x) < get<ustr>(y); };

    ustr_type.iter = [this](auto &in) {
      return IterRef(new UStrIter(*this, get<ustr>(in)));
    };
    
    rat_type.supers.push_back(&any_type);
    rat_type.supers.push_back(&ordered_type);
    rat_type.fmt = [](auto &v) { return fmt_arg(get<Rat>(v)); };
    rat_type.eq = [](auto &x, auto &y) { return get<Rat>(x) == get<Rat>(y); };
    rat_type.lt = [](auto &x, auto &y) { return get<Rat>(x) < get<Rat>(y); };

    uid_type.supers.push_back(&any_type);
    uid_type.supers.push_back(&ordered_type);
    uid_type.fmt = [](auto &v) { return fmt("Uid(%0)", get<Uid>(v)); };
    uid_type.eq = [](auto &x, auto &y) { return get<Uid>(x) == get<Uid>(y); };
    uid_type.lt = [](auto &x, auto &y) { return get<Uid>(x) < get<Uid>(y); };

    random_type.supers.push_back(&any_type);
    random_type.supers.push_back(&get_iterable_type(*this, i64_type));
    
    random_type.fmt = [](auto &v) {
      auto &r(*get<RandomRef>(v));
      return fmt("Random(%0:%1)", r.a(), r.b());
    };
    
    random_type.eq = [](auto &x, auto &y) {
      return get<RandomRef>(x) == get<RandomRef>(y);
    };

    random_type.iter = [this](auto &in) {
      return IterRef(new RandomIter(*this, get<RandomRef>(in)));
    };

    thread_type.supers.push_back(&any_type);
    thread_type.fmt = [](auto &v) { return fmt("thread_%0", get<Thread *>(v)->id); };
    thread_type.eq = [](auto &x, auto &y) {
      return get<Thread *>(x) == get<Thread *>(y);
    };

    for (int i(0); i < MAX_TARGET; i++) {
      return_target[i]->return_depth = i+1;
      recall_target[i]->recall_depth = i+1;
      yield_target[i]->yield_depth = i+1;
      break_target[i]->break_depth = i+1;
    }

    init_opts(*this);
    init_pairs(*this);
    init_lists(*this);
    init_tables(*this);
    init_procs(*this);
    
    add_conv(*this, str_type, ustr_type, [this](auto &v) {	
	v.type = &ustr_type;
	v.val = uconv.from_bytes(get<str>(v));
	return true;
      });

    add_conv(*this, str_type, path_type, [this](auto &v) {	
	v.type = &path_type;
	v.val = Path(get<str>(v));
	return true;
      });

    add_conv(*this, i64_type, rat_type, [this](auto &v) {	
	v.type = &rat_type;
	auto n(get<int64_t>(v));
	v.val = Rat(abs(n), 1, n < 0);
	return true;
      });

    add_conv(*this, rwfile_type, rfile_type, [this](auto &v) {	
	v.type = &rfile_type;
	return true;
      });
    
    add_func(*this, "is?",
	     {ArgType(any_type), ArgType(meta_type)}, {ArgType(bool_type)},
	     is_imp);
   
    add_func(*this, "type",
	     {ArgType(any_type)}, {ArgType(0)},
	     type_imp);
   
    add_func(*this, "=",
	     {ArgType(any_type), ArgType(0)}, {ArgType(bool_type)},
	     eq_imp);
   
    add_func(*this, "==",
	     {ArgType(any_type), ArgType(0)}, {ArgType(bool_type)},
	     equal_imp);
   
    add_func(*this, "lt?",
	     {ArgType(ordered_type), ArgType(0)}, {ArgType(bool_type)},
	     lt_imp);

    add_func(*this, "gt?",
	     {ArgType(ordered_type), ArgType(0)}, {ArgType(bool_type)},
	     gt_imp);

    add_func(*this, "when",
	     {ArgType(bool_type), ArgType(callable_type)}, {},
	     when_imp);

    add_func(*this, "unless",
	     {ArgType(bool_type), ArgType(callable_type)}, {},
	     unless_imp);

    add_func(*this, "z?",
	     {ArgType(i64_type)}, {ArgType(bool_type)},
	     zero_i64_imp);
    
    add_func(*this, "+?",
	     {ArgType(i64_type)}, {ArgType(bool_type)},
	     pos_i64_imp);

    add_func(*this, "+1",
	     {ArgType(i64_type)}, {ArgType(i64_type)},
	     inc_i64_imp);

    add_func(*this, "-1",
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
    add_func(*this, "/",
	     {ArgType(i64_type), ArgType(i64_type)}, {ArgType(rat_type)},
	     div_i64_imp);
    add_func(*this, "%",
	     {ArgType(i64_type), ArgType(i64_type)}, {ArgType(i64_type)},
	     mod_i64_imp);

    add_func(*this, "str",
	     {ArgType(i64_type)}, {ArgType(str_type)},
	     i64_str_imp);

    add_func(*this, "trunc",
	     {ArgType(rat_type)}, {ArgType(i64_type)},
	     trunc_imp);
    add_func(*this, "frac",
	     {ArgType(rat_type)}, {ArgType(rat_type)},
	     frac_imp);
    add_func(*this, "+",
	     {ArgType(rat_type), ArgType(rat_type)}, {ArgType(rat_type)},
	     add_rat_imp);
    add_func(*this, "-",
	     {ArgType(rat_type), ArgType(rat_type)}, {ArgType(rat_type)},
	     sub_rat_imp);
    add_func(*this, "*",
	     {ArgType(rat_type), ArgType(rat_type)}, {ArgType(rat_type)},
	     mul_rat_imp);
    add_func(*this, "/",
	     {ArgType(rat_type), ArgType(rat_type)}, {ArgType(rat_type)},
	     div_rat_imp);

    add_func(*this, "z?",
	     {ArgType(bin_type)}, {ArgType(bool_type)},
	     bin_zero_imp);
    
    add_func(*this, "+?",
	     {ArgType(bin_type)}, {ArgType(bool_type)},
	     bin_pos_imp);
    
    add_func(*this, "bytes",
	     {ArgType(i64_type)}, {ArgType(bin_type)},
	     bytes_imp);

    add_func(*this, "len",
	     {ArgType(bin_type)}, {ArgType(i64_type)},
	     bin_len_imp);

    add_func(*this, "str",
	     {ArgType(bin_type)}, {ArgType(str_type)},
	     bin_str_imp);

    add_func(*this, "ustr",
	     {ArgType(bin_type)}, {ArgType(ustr_type)},
	     bin_ustr_imp);

    add_func(*this, "append",
	     {ArgType(bin_type), ArgType(bin_type)},
	     {ArgType(bin_type)},
	     bin_append_imp);

    add_func(*this, "len",
	     {ArgType(str_type)}, {ArgType(i64_type)},
	     str_len_imp);

    add_func(*this, "len",
	     {ArgType(ustr_type)}, {ArgType(i64_type)},
	     ustr_len_imp);

    add_func(*this, "bytes",
	     {ArgType(str_type)}, {ArgType(bin_type)},
	     str_bytes_imp);

    add_func(*this, "ustr",
	     {ArgType(str_type)}, {ArgType(ustr_type)},
	     str_ustr_imp);

    add_func(*this, "bytes",
	     {ArgType(ustr_type)}, {ArgType(bin_type)},
	     ustr_bytes_imp);

    add_func(*this, "str",
	     {ArgType(ustr_type)}, {ArgType(str_type)},
	     ustr_str_imp);

    add_func(*this, "uid",
	     {}, {ArgType(uid_type)},
	     uid_imp);

    add_func(*this, "iter",
	     {ArgType(iterable_type)},
	     {ArgType([this](auto &args) { 
		   return &get_iter_type(*this, *args.at(0).type->args.at(0)); 
		 })},
	     iter_imp);

    add_func(*this, "str",
	     {ArgType(get_iterable_type(*this, char_type))},
	     {ArgType(str_type)},
	     iterable_str_imp);

    add_func(*this, "join",
	     {ArgType(iterable_type), ArgType(any_type)}, {ArgType(str_type)},
	     iterable_join_str_imp);

    add_func(*this, "filter",
	     {ArgType(iterable_type), ArgType(callable_type)},
	     {ArgType([this](auto &args) {
		   return &get_iter_type(*this, *args.at(0).type->args.at(0));
		 })},			
	     iterable_filter_imp);

    add_func(*this, "map",
	     {ArgType(iterable_type), ArgType(callable_type)},
	     {ArgType(iter_type)},			
	     iterable_map_imp);
        
    add_func(*this, "rfile",
	     {ArgType(path_type)}, {ArgType(rfile_type)},
	     rfile_imp);

    add_func(*this, "rwfile",
	     {ArgType(path_type)}, {ArgType(rwfile_type)},
	     rwfile_imp);

    add_func(*this, "read",
	     {ArgType(readable_type)},
	     {ArgType(get_iter_type(*this, get_opt_type(*this, bin_type)))},
	     read_imp);

    add_func(*this, "write",
	     {ArgType(get_iterable_type(*this, bin_type)), ArgType(writeable_type)},
	     {ArgType(get_iter_type(*this, i64_type))},
	     write_imp);

    add_func(*this, "lines",
	     {ArgType(get_iterable_type(*this, bin_type))},
	     {ArgType(get_iter_type(*this, get_opt_type(*this, str_type)))},	
	     iterable_lines_imp);

    add_func(*this, "words",
	     {ArgType(get_iterable_type(*this, bin_type))},
	     {ArgType(get_iter_type(*this, get_opt_type(*this, str_type)))},	
	     iterable_words_imp);

    add_func(*this, "random",
	     {ArgType(i64_type)}, {ArgType(random_type)},
	     random_imp);

    add_func(*this, "pop",
	     {ArgType(random_type)}, {ArgType(i64_type)},
	     random_pop_imp);

    add_func(*this, "proc",
	     {ArgType(coro_type)}, {ArgType(proc_type)},
	     proc_imp);

    add_func(*this, "run",
	     {ArgType(proc_type)}, {},
	     proc_run_imp);

    add_func(*this, "thread",
	     {ArgType(callable_type)}, {ArgType(thread_type)},
	     thread_imp);

    add_func(*this, "join",
	     {ArgType(thread_type)}, {ArgType(any_type)},
	     thread_join_imp);

    add_macro(*this, "!", [](auto pos, auto &in, auto &out) {	
	out.emplace_back(Check());
      });
    
    add_macro(*this, "{", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Lambda());
      });

    add_macro(*this, "}", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Unlambda());
      });

    add_macro(*this, "(", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Backup(true));
      });

    add_macro(*this, ")", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Restore());
      });

    add_macro(*this, "[", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Backup());
      });

    add_macro(*this, "]", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Stash());	
	out.emplace_back(Restore());
      });

    add_macro(*this, "<", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Param());
      });

    add_macro(*this, ">", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Unparam());
      });

    add_macro(*this, "$", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Dup());
      });

    add_macro(*this, "_", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Drop(1));
      });

    add_macro(*this, "|", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Reset());
      });
    
    add_macro(*this, "func:", [this](auto pos, auto &in, auto &out) {
	if (in.size() < 2) {
	  ERROR(Snabel, fmt("Malformed func on row %0, col %1",
			    pos.row, pos.col));
	} else {
	  out.emplace_back(Backup(true));
	  const str n(in.at(0).text);
	  auto start(std::next(in.begin()));
	  auto end(find_end(start, in.end()));
	  compile(*this, TokSeq(start, end), out);
	  if (end != in.end()) { end++; }
	  in.erase(in.begin(), end);
	  out.emplace_back(Restore());
	  out.emplace_back(Putenv(n));
	}
      });

    add_macro(*this, "label:", [this](auto pos, auto &in, auto &out) {
	if (in.empty()) {
	  ERROR(Snabel, fmt("Malformed label on row %0, col %1",
			    pos.row, pos.col));
	  return;
	}

	const str tag(in.at(0).text);
	in.pop_front();

	if (!in.empty()) {
	  if (in.at(0).text != ";") {
	    ERROR(Snabel, fmt("Malformed label on row %0, col %1",
				pos.row, pos.col));
	    return;
	  }

	  in.pop_front();
	}

	out.emplace_back(Target(add_label(*this, tag)));
      });
    
    add_macro(*this, "let:", [this](auto pos, auto &in, auto &out) {
	if (in.empty()) {
	  ERROR(Snabel, fmt("Malformed let on row %0, col %1",
			    pos.row, pos.col));
	} else {
	  out.emplace_back(Backup(true));
	  const str n(in.at(0).text);
	  auto start(std::next(in.begin()));
	  auto end(find_end(start, in.end()));
	  compile(*this, TokSeq(start, end), out);
	  if (end != in.end()) { end++; }
	  in.erase(in.begin(), end);
	  out.emplace_back(Restore());
	  out.emplace_back(Putenv(fmt("@%0", n)));
	}
      });

    add_macro(*this, "$list", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Stash());
      });

    add_macro(*this, "recall", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Recall(1));
      });

    add_macro(*this, "return", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Return(1));
      });

    add_macro(*this, "yield", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Yield(1));
      });

    add_macro(*this, "break", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Break(1));
      });

    add_macro(*this, "call", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Call());
      });

    add_macro(*this, "for", [](auto pos, auto &in, auto &out) {
	out.emplace_back(For(true));
      });

    add_macro(*this, "_for", [](auto pos, auto &in, auto &out) {
	out.emplace_back(For(false));
      });

    add_macro(*this, "while", [](auto pos, auto &in, auto &out) {
	out.emplace_back(While());
      });
  }

  Macro &add_macro(Exec &exe, const str &n, Macro::Imp imp) {
    return exe.macros.emplace(std::piecewise_construct,
			      std::forward_as_tuple(n),
			      std::forward_as_tuple(n, imp)).first->second; 
  }
  
  Type &get_meta_type(Exec &exe, Type &t) {    
    str n(fmt("Type<%0>", t.name));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &mt(add_type(exe, n));
    mt.raw = &exe.meta_type;
    mt.supers.push_back(&exe.meta_type);
    mt.args.push_back(&t);
    mt.fmt = [](auto &v) { return get<Type *>(v)->name; };
    mt.eq = [](auto &x, auto &y) { return get<Type *>(x) == get<Type *>(y); };
    return mt;
  }

  Type &add_type(Exec &exe, const str &n) {
    return exe.types.emplace(std::piecewise_construct,
			     std::forward_as_tuple(n),
			     std::forward_as_tuple(n)).first->second; 
  }

  Type *find_type(Exec &exe, const str &n) {
    auto fnd(exe.types.find(n));
    if (fnd == exe.types.end()) { return nullptr; }
    return &fnd->second;
  }

  Type &get_type(Exec &exe, Type &raw, Types args) {
    if (args.empty()) { return raw; }
    
    if (args.size() > raw.args.size()) {
      ERROR(Snabel, fmt("Too many params for type %0", raw.name));
      return raw;
    }

    for (size_t i(args.size()); i < raw.args.size(); i++) {
      args.push_back(raw.args.at(i));
    }
		  
    if (&raw == &exe.iter_type) {
      return get_iter_type(exe, *args.at(0));
    } else if (&raw == &exe.iterable_type) {      
      return get_iterable_type(exe, *args.at(0));
    } else if (&raw == &exe.list_type) {      
      return get_list_type(exe, *args.at(0));
    } else if (&raw == &exe.table_type) {      
      return get_table_type(exe, *args.at(0), *args.at(1));
    } else if (&raw == &exe.opt_type) {      
      return get_opt_type(exe, *args.at(0));
    } else if (&raw == &exe.pair_type) {      
      return get_pair_type(exe, *args.at(0), *args.at(1));
    }

    ERROR(Snabel, fmt("Invalid type: %1", raw.name));
    return raw;
  }

  Type *get_super(Exec &exe, Type &raw, const Types &x, const Types &y) {
    if (x.size() != y.size()) { return &raw; }
    Types args;
    
    for (auto i(x.begin()), j(y.begin()); i != x.end() && j != y.end(); i++, j++) {
      auto s(get_super(exe, **i, **j));
      args.push_back(s ? s : &exe.any_type);
    }

    return &get_type(exe, raw, args);
  }

  Type *get_super(Exec &exe, Type &x, Type &y) {
    if (&x == &y) { return &x; }

    if (x.raw && x.raw == y.raw) {
      return get_super(exe, *x.raw, x.args, y.args);
    }
    
    for (auto i(x.supers.rbegin()); i != x.supers.rend(); i++) {
      for (auto j(y.supers.rbegin()); j != y.supers.rend(); j++) {
	auto res(get_super(exe, **i, **j));
	if (res) { return res; }
      }
    }

    return nullptr;
  }

  Type *get_sub(Exec &exe, Type &t, Type &raw) {
    if (t.raw == &raw) { return &t; }
    
    for (auto i(t.supers.rbegin()); i != t.supers.rend(); i++) {
      auto res(get_sub(exe, **i, raw));
      if (res) { return res; }
    }

    return nullptr;
  }
  
  Type &get_iter_type(Exec &exe, Type &elt) {    
    str n(fmt("Iter<%0>", elt.name));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.iter_type;
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&get_iterable_type(exe, elt));    
    t.supers.push_back(&exe.iter_type);
    t.args.push_back(&elt);
    t.fmt = exe.iter_type.fmt;
    t.eq = exe.iter_type.eq;
    t.iter = exe.iter_type.iter;
    return t;
  }

  Type &get_iterable_type(Exec &exe, Type &elt) {    
    str n(fmt("Iterable<%0>", elt.name));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.iterable_type;
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&exe.iterable_type);
    t.args.push_back(&elt);
    t.fmt = exe.iterable_type.fmt;
    t.eq = exe.iterable_type.eq;
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

  Label &add_label(Exec &exe, const str &tag, bool pmt) {
    auto &l(exe.labels
      .emplace(std::piecewise_construct,
	       std::forward_as_tuple(tag),
	       std::forward_as_tuple(exe, tag, pmt))
	    .first->second);
    put_env(exe.main_scope, tag, Box(exe.label_type, &l));
    return l;
  }

  void clear_labels(Exec &exe) {
    for (auto i(exe.labels.begin()); i != exe.labels.end();) {
      auto &l(i->second);
      if (l.permanent) {
	i++;
      } else {
	rem_env(exe.main_scope, l.tag);
	i = exe.labels.erase(i);
      }
    }
  }

  Label *find_label(Exec &exe, const str &tag) {
    auto fnd(exe.labels.find(tag));
    if (fnd == exe.labels.end()) { return nullptr; }
    return &fnd->second;
  }

  void add_conv(Exec &exe, Type &from, Type &to, Conv conv) {
    from.conv = true;
    exe.convs.emplace(std::piecewise_construct,
		      std::forward_as_tuple(&from, &to),
		      std::forward_as_tuple(conv));
  }
  
  bool conv(Exec &exe, Box &val, Type &type) {
    if (!val.type->conv) { return false; }
    auto fnd(exe.convs.find(std::make_pair(val.type, &type)));
    if (fnd == exe.convs.end()) { return false; }
    return fnd->second(val);
  }

  Uid uid(Exec &exe) {
    return exe.next_uid.fetch_add(1);
  }

  Box make_opt(Exec &exe, opt<Box> in) {
    return in
      ? Box(get_opt_type(exe, *in->type), in->val)
      : Box(exe.opt_type, n_a);
  }

  void rewind(Exec &exe) {
    for (auto i(exe.threads.begin()); i != exe.threads.end();) {
      if (i->first == exe.main.id) {
	i++;
      } else {
	i = exe.threads.erase(i);
      }
    }

    auto &thd(exe.main);
    while (thd.scopes.size() > 1) { thd.scopes.pop_back(); }
    while (thd.stacks.size() > 1) { thd.stacks.pop_back(); }
    thd.main_scope.recalls.clear();
    thd.main_scope.return_pc = -1;
    thd.stacks.front().clear();
    thd.pc = 0;
  }

  bool compile(Exec &exe, const str &in) {
    Exec::Lock lock(exe.mutex);
    
    exe.main.ops.clear();
    clear_labels(exe);
    size_t lnr(0);
    TokSeq toks;
    
    for (auto &ln: parse_lines(in)) {
      if (!ln.empty()) { parse_expr(ln, lnr, toks); }
      lnr++;
    }

    compile(exe, toks, exe.main.ops);
    TRY(try_compile);

    while (true) {
      rewind(exe);
      
      for (auto &op: exe.main.ops) {
	if ((!op.prepared && !prepare(op, exe.main_scope)) ||
	    !try_compile.errors.empty()) {
	  goto exit;
	}
	
	exe.main.pc++;
      }

      exe.main.pc = 0;
      exe.lambdas.clear();
      bool done(false);
      
      while (!done) {
	done = true;
	
	for (auto &op: exe.main.ops) {
	  if (refresh(op, exe.main_scope)) { done = false; }
	  if (!try_compile.errors.empty()) { goto exit; }
	  exe.main.pc++;
	}
      }

      OpSeq out;
      done = true;

      for (auto &op: exe.main.ops) {
	if (compile(op, exe.main_scope, out)) { done = false; }
	if (!try_compile.errors.empty()) { goto exit; }
      }

      if (done) { break; }
      exe.main.ops.clear();
      exe.main.ops.swap(out);
      try_compile.errors.clear();
    }

    {
      OpSeq out;
      exe.main.pc = 0;
      
      for (auto &op: exe.main.ops) {
	if (!finalize(op, exe.main_scope, out)) {
	  exe.main.pc++;
	}
	
	if (!try_compile.errors.empty()) { goto exit; }
      }
      
      exe.main.ops.clear();
      exe.main.ops.swap(out);
    }
  exit:
    rewind(exe);
    return try_compile.errors.empty();
  }
  
  bool run(Exec &exe, const str &in) {
    rewind(exe);
    if (!compile(exe, in)) { return false; }
    return run(exe.main);
  }
}
