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
#include "snabel/proc.hpp"
#include "snabel/range.hpp"
#include "snabel/str.hpp"
#include "snabel/type.hpp"

namespace snabel {
  static void nop_imp(Scope &scp, const Args &args)
  { }

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

  static void opt_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, get_opt_type(scp.exec, *in.type), in.val);
  }

  static void opt_or_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto &alt(args.at(1));

    if (empty(in)) {
      push(scp.thread, alt);
    } else {
      push(scp.thread, *in.type->args.at(0), in.val);
    }
  }

  static void opt_or_opt_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto &alt(args.at(1));
    push(scp.thread, empty(in) ? alt : in);
  }

  static void zero_i64_imp(Scope &scp, const Args &args) {
    bool res(get<int64_t>(args.at(0)) == 0);
    push(scp.thread, scp.exec.bool_type, res);
  }

  static void pos_i64_imp(Scope &scp, const Args &args) {
    bool res(get<int64_t>(args.at(0)) > 0);
    push(scp.thread, scp.exec.bool_type, res);
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

  static void bin_append_io_buf_imp(Scope &scp, const Args &args) {
    auto &out(args.at(0));
    auto &tgt(*get<BinRef>(out));
    auto &src(*get<IOBufRef>(args.at(1)));
    std::copy(src.data.begin(), std::next(src.data.begin(), src.rpos),
	      std::back_inserter(tgt));
    push(scp.thread, out);
  }

  static void io_buf_len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp.thread, in);
    push(scp.thread, scp.exec.i64_type, (int64_t)get<IOBufRef>(in)->rpos);
  }

  static void io_queue_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.io_queue_type, std::make_shared<IOQueue>());
  }

  static void io_queue_push_buf_imp(Scope &scp, const Args &args) {
    auto &q(args.at(0));
    auto &b(args.at(1));
    get<IOQueueRef>(q)->bufs.push_back(*get<IOBufRef>(b));
    push(scp.thread, q);
  }

  static void io_queue_push_bin_imp(Scope &scp, const Args &args) {
    auto &q(args.at(0));
    auto &b(args.at(1));
    get<IOQueueRef>(q)->bufs.emplace_back(*get<BinRef>(b));
    push(scp.thread, q);
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

  static void ustr_bytes_imp(Scope &scp, const Args &args) {
    auto in(uconv.to_bytes(get<ustr>(args.at(0))));
    push(scp.thread, scp.exec.bin_type, std::make_shared<Bin>(in.begin(), in.end()));
  }

  static void str_ustr_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.ustr_type, uconv.from_bytes(get<str>(args.at(0))));
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

  static void iterable_list_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    auto out(std::make_shared<List>());
    
    while (true) {
      auto v(it->next(scp));
      if (!v) { break; }
      out->push_back(*v);
    }
    
    push(scp.thread, get_list_type(scp.exec, *it->type.args.at(0)), out); 
  }

  static void iterable_filter_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0)), &tgt(args.at(1));
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 Iter::Ref(new FilterIter(exe, it, elt, tgt)));
  }

  static void iterable_map_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0)), &tgt(args.at(1));
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0));
    
    push(scp.thread,
	 get_iter_type(exe, elt),
	 Iter::Ref(new MapIter(exe, it, elt, tgt)));
  }

  static void iterable_zip_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &x(args.at(0)), &y(args.at(1));
    auto xi((*x.type->iter)(x)), yi((*y.type->iter)(y));

    push(scp.thread,
	 get_iter_type(exe, get_pair_type(exe,
					  *xi->type.args.at(0),
					  *yi->type.args.at(0))),
	 Iter::Ref(new ZipIter(exe, xi, yi)));
  }

  static void list_imp(Scope &scp, const Args &args) {
    auto &elt(args.at(0));
    push(scp.thread,
	 get_list_type(scp.exec, *get<Type *>(elt)),
	 std::make_shared<List>());    
  }

  static void list_push_imp(Scope &scp, const Args &args) {
    auto &lst(args.at(0));
    auto &el(args.at(1));
    get<ListRef>(lst)->push_back(el);
    push(scp.thread, lst);    
  }

  static void list_pop_imp(Scope &scp, const Args &args) {
    auto &lst_arg(args.at(0));
    auto &lst(*get<ListRef>(lst_arg));
    push(scp.thread, lst_arg);
    push(scp.thread, lst.back());
    lst.pop_back();
  }

  static void list_reverse_imp(Scope &scp, const Args &args) {
    auto &in_arg(args.at(0));
    auto &in(*get<ListRef>(in_arg));
    std::reverse(in.begin(), in.end());
    push(scp.thread, in_arg); 
  }

  static void list_unzip_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0));

    auto &yt(*in.type->args.at(0)->args.at(0));
    push(scp.thread,
	 get_iter_type(exe, yt),
	 Iter::Ref(new ListIter(exe, yt, get<ListRef>(in), [](auto &el) {
	       return get<PairRef>(el)->first;
	     })));

    auto &xt(*in.type->args.at(0)->args.at(1));
    push(scp.thread,
	 get_iter_type(exe, xt),
	 Iter::Ref(new ListIter(exe, xt, get<ListRef>(in), [](auto &el) {
	       return get<PairRef>(el)->second;
	     })));
  }

  static void zip_imp(Scope &scp, const Args &args) {
    auto &l(args.at(0)), &r(args.at(1));
    
    push(scp.thread,
	 get_pair_type(scp.exec, *l.type, *r.type),
	 std::make_shared<Pair>(l, r));    
  }

  static void unzip_imp(Scope &scp, const Args &args) {
    auto &p(*get<PairRef>(args.at(0)));   
    push(scp.thread, p.first);
    push(scp.thread, p.second);
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
    
    push(scp.thread,
	 get_iter_type(exe, exe.bin_type),
	 Iter::Ref(new ReadIter(exe, args.at(0))));
  }

  static void write_imp(Scope &scp, const Args &args) {
    Exec &exe(scp.exec);
    auto &out(args.at(0));
    push(scp.thread, out);
    push(scp.thread,
	 get_iter_type(exe, exe.i64_type),
	 Iter::Ref(new WriteIter(exe, get<IOQueueRef>(args.at(1)), out)));
  }

  static void proc_imp(Scope &scp, const Args &args) {
    push(scp.thread, scp.exec.proc_type,
	 std::make_shared<Proc>(*get<Label *>(args.at(0))));
  }

  static void proc_run_imp(Scope &scp, const Args &args) {
    auto &p(*get<ProcRef>(args.at(0)));
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
    file_type(add_type(*this, "File")),    
    func_type(add_type(*this, "Func")),
    i64_type(add_type(*this, "I64")),
    io_buf_type(add_type(*this, "IOBuf")),
    io_queue_type(add_type(*this, "IOQueue")),
    iter_type(add_type(*this, "Iter")),
    iterable_type(add_type(*this, "Iterable")),
    label_type(add_type(*this, "Label")),
    lambda_type(add_type(*this, "Lambda")),
    list_type(add_type(*this, "List")),
    opt_type(add_type(*this, "Opt")),
    ordered_type(add_type(*this, "Ordered")),
    pair_type(add_type(*this, "Pair")),
    path_type(add_type(*this, "Path")),
    proc_type(add_type(*this, "Proc")),    
    readable_type(add_type(*this, "Readable")),
    rfile_type(add_type(*this, "RFile")),
    rat_type(add_type(*this, "Rat")),
    rwfile_type(add_type(*this, "RWFile")),    
    str_type(add_type(*this, "Str")),
    thread_type(add_type(*this, "Thread")),
    uchar_type(add_type(*this, "UChar")),
    uid_type(add_type(*this, "Uid")),
    ustr_type(add_type(*this, "UStr")),
    void_type(add_type(*this, "Void")),
    writeable_type(add_type(*this, "Writeable")),
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

    opt_type.supers.push_back(&any_type);
    opt_type.args.push_back(&any_type);
    opt_type.dump = [](auto &v) -> str {
      if (empty(v)) { return "#n/a"; }
      return fmt("Opt(%0)", v.type->args.at(0)->dump(v));
    };
    opt_type.fmt = [](auto &v) -> str {
      if (empty(v)) { return "#n/a"; }
      return fmt("Opt(%0)", v.type->args.at(0)->fmt(v));
    };
    opt_type.eq = [](auto &x, auto &y) {
      if (empty(x) && !empty(y)) { return true; }
      if (!empty(x) || !empty(y)) { return false; }
      return x.type->eq(x, y);
    };
    opt_type.equal = [](auto &x, auto &y) {
      if (empty(x) && !empty(y)) { return true; }
      if (!empty(x) || !empty(y)) { return false; }
      return x.type->equal(x, y);
    };
    put_env(main_scope, "#n/a", Box(opt_type, empty_val));

    callable_type.supers.push_back(&any_type);
    callable_type.args.push_back(&any_type);

    iter_type.supers.push_back(&any_type);
    iter_type.args.push_back(&any_type);
    iter_type.fmt = [](auto &v) { return fmt("Iter<%0>", v.type->args.at(0)->name); };
    iter_type.eq = [](auto &x, auto &y) {
      return get<Iter::Ref>(x) == get<Iter::Ref>(y);
    };
    iter_type.iter = [](auto &in) { return get<Iter::Ref>(in); };

    iterable_type.supers.push_back(&any_type);
    iterable_type.args.push_back(&any_type);
    iterable_type.fmt = [](auto &v) {
      return fmt("Iterable<%0>", v.type->args.at(0)->name);
    };
    iterable_type.eq = [](auto &x, auto &y) { return false; };

    list_type.supers.push_back(&any_type);
    list_type.args.push_back(&any_type);
    list_type.dump = [](auto &v) { return dump(*get<ListRef>(v)); };
    list_type.fmt = [](auto &v) { return list_fmt(*get<ListRef>(v)); };
    list_type.eq = [](auto &x, auto &y) {
      return get<ListRef>(x) == get<ListRef>(y);
    };
    list_type.iter = [this](auto &in) {
      return Iter::Ref(new ListIter(*this, *in.type->args.at(0), get<ListRef>(in)));
    };

    list_type.equal = [](auto &x, auto &y) {
      auto &xs(*get<ListRef>(x)), &ys(*get<ListRef>(y));
      if (xs.size() != ys.size()) { return false; }
      auto xi(xs.begin()), yi(ys.begin());

      for (; xi != xs.end() && yi != ys.end(); xi++, yi++) {
	if (xi->type != yi->type || !xi->type->equal(*xi, *yi)) { return false; }
      }

      return true;
    };

    pair_type.supers.push_back(&any_type);
    pair_type.args.push_back(&any_type);
    pair_type.args.push_back(&any_type);
    pair_type.dump = [](auto &v) { return dump(*get<PairRef>(v)); };
    pair_type.fmt = [](auto &v) { return pair_fmt(*get<PairRef>(v)); };
    pair_type.eq = [](auto &x, auto &y) { 
      return get<PairRef>(x) == get<PairRef>(y); 
    };
    pair_type.equal = [](auto &x, auto &y) { 
      return *get<PairRef>(x) == *get<PairRef>(y); 
    };
    pair_type.lt = [](auto &x, auto &y) { 
      return *get<PairRef>(x) < *get<PairRef>(y); 
    };

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
      return Iter::Ref(new BinIter(*this, get<BinRef>(in)));
    };

    io_buf_type.supers.push_back(&any_type);
    io_buf_type.fmt = [](auto &v) {
      auto &b(*get<IOBufRef>(v));
      return fmt("IOBuf(%0:%1)", b.rpos, b.data.size());
    };
    io_buf_type.eq = [](auto &x, auto &y) {
      return get<IOBufRef>(x) == get<IOBufRef>(y);
    };
    io_buf_type.equal = [](auto &x, auto &y) {
      return *get<IOBufRef>(x) == *get<IOBufRef>(y);
    };

    io_queue_type.supers.push_back(&any_type);
    io_queue_type.fmt = [](auto &v) {
      auto &q(*get<IOQueueRef>(v));
      return fmt("IOQueue(%0)", q.bufs.size());
    };
    io_queue_type.eq = [](auto &x, auto &y) {
      return get<IOQueueRef>(x) == get<IOQueueRef>(y);
    };
    io_queue_type.equal = [](auto &x, auto &y) {
      return *get<IOQueueRef>(x) == *get<IOQueueRef>(y);
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
	call(*get<ProcRef>(v), scp, now);
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
      auto res(read(f.fd, &out.data[out.rpos], out.data.size()-out.rpos));
      if (!res) { return false; }

      if (res == -1) {
	if (errno == EAGAIN) { return true; }
	ERROR(Snabel, fmt("Failed reading from file: %0", errno));
	return false;
      }

      out.rpos += res;
      return true;
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
      return Iter::Ref(new RangeIter(*this, Range(0, get<int64_t>(in))));
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

    str_type.supers.push_back(&any_type);
    str_type.supers.push_back(&ordered_type);
    str_type.supers.push_back(&get_iterable_type(*this, char_type));
    str_type.fmt = [](auto &v) { return fmt("'%0'", get<str>(v)); };
    str_type.eq = [](auto &x, auto &y) { return get<str>(x) == get<str>(y); };
    str_type.lt = [](auto &x, auto &y) { return get<str>(x) < get<str>(y); };

    str_type.iter = [this](auto &in) {
      return Iter::Ref(new StrIter(*this, get<str>(in)));
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
      return Iter::Ref(new UStrIter(*this, get<ustr>(in)));
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

    thread_type.supers.push_back(&any_type);
    thread_type.fmt = [](auto &v) { return fmt("thread_%0", get<Thread *>(v)->id); };
    thread_type.eq = [](auto &x, auto &y) {
      return get<Thread *>(x) == get<Thread *>(y);
    };

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
    
    add_func(*this, "nop", {}, {}, nop_imp);

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

    add_func(*this, "opt",
	     {ArgType(any_type)},
	     {ArgType([this](auto &args) {
		   return &get_opt_type(*this, *args.at(0).type);
		 })},
	     opt_imp);

    add_func(*this, "or",
	     {ArgType(opt_type),
		 ArgType([](auto &args) { return args.at(0).type->args.at(0); })},
	     {ArgType([](auto &args) { return args.at(0).type->args.at(0); })},
	     opt_or_imp);

    add_func(*this, "or",
	     {ArgType(opt_type), ArgType(0)}, {ArgType(0)},
	     opt_or_opt_imp);

    add_func(*this, "z?",
	     {ArgType(i64_type)}, {ArgType(bool_type)},
	     zero_i64_imp);
    
    add_func(*this, "+?",
	     {ArgType(i64_type)}, {ArgType(bool_type)},
	     pos_i64_imp);
    
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

    add_func(*this, "bytes",
	     {ArgType(i64_type)}, {ArgType(bin_type)},
	     bytes_imp);

    add_func(*this, "len",
	     {ArgType(bin_type)}, {ArgType(i64_type)},
	     bin_len_imp);

    add_func(*this, "len",
	     {ArgType(io_buf_type)}, {ArgType(i64_type)},
	     io_buf_len_imp);

    add_func(*this, "io-queue",
	     {}, {ArgType(io_queue_type)},
	     io_queue_imp);

    add_func(*this, "push",
	     {ArgType(io_queue_type), ArgType(io_buf_type)},
	     {ArgType(io_queue_type)},
	     io_queue_push_buf_imp);

    add_func(*this, "push",
	     {ArgType(io_queue_type), ArgType(bin_type)},
	     {ArgType(io_queue_type)},
	     io_queue_push_bin_imp);

    add_func(*this, "str",
	     {ArgType(bin_type)}, {ArgType(str_type)},
	     bin_str_imp);

    add_func(*this, "ustr",
	     {ArgType(bin_type)}, {ArgType(ustr_type)},
	     bin_ustr_imp);

    add_func(*this, "append",
	     {ArgType(bin_type), ArgType(io_buf_type)}, {ArgType(bin_type)},
	     bin_append_io_buf_imp);

    add_func(*this, "len",
	     {ArgType(str_type)}, {ArgType(i64_type)},
	     str_len_imp);

    add_func(*this, "len",
	     {ArgType(ustr_type)}, {ArgType(i64_type)},
	     ustr_len_imp);

    add_func(*this, "bytes",
	     {ArgType(str_type)}, {ArgType(bin_type)},
	     str_bytes_imp);

    add_func(*this, "bytes",
	     {ArgType(ustr_type)}, {ArgType(bin_type)},
	     ustr_bytes_imp);

    add_func(*this, "ustr",
	     {ArgType(str_type)}, {ArgType(ustr_type)},
	     str_ustr_imp);

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

    add_func(*this, "list",
	     {ArgType(iterable_type)},
	     {ArgType([this](auto &args) {
		   return &get_list_type(*this, *args.at(0).type->args.at(0));
		 })},
	     iterable_list_imp);

    add_func(*this, "zip",
	     {ArgType(iterable_type), ArgType(iterable_type)},
	     {ArgType([this](auto &args) {
		   return &get_iter_type(*this,
					 get_pair_type(*this,
						       *args.at(0).type->args.at(0),
						       *args.at(1).type->args.at(0)));
		 })},			
	     iterable_zip_imp);

    add_func(*this, "filter",
	     {ArgType(iterable_type), ArgType(callable_type)},
	     {ArgType([this](auto &args) {
		   return &get_iter_type(*this, *args.at(0).type->args.at(0));
		 })},			
	     iterable_filter_imp);

    add_func(*this, "map",
	     {ArgType(iterable_type), ArgType(callable_type)},
	     {ArgType([this](auto &args) {
		   return &get_iter_type(*this, *args.at(0).type->args.at(0));
		 })},			
	     iterable_map_imp);

    add_func(*this, "list",
	     {ArgType(meta_type)},
	     {ArgType([this](auto &args) {
		   return &get_list_type(*this, *args.at(0).type);
		 })},
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

    add_func(*this, "unzip",
	     {ArgType(get_list_type(*this, pair_type))},
	     {ArgType([this](auto &args) {
		   return &get_iter_type(*this,
					 *args.at(0).type->args.at(0)->args.at(0));
		 }),
		 ArgType([this](auto &args) {
		     return &get_iter_type(*this,
					   *args.at(0).type->args.at(0)->args.at(1));
		   })},			
	     list_unzip_imp);

    add_func(*this, ".",
	     {ArgType(any_type), ArgType(any_type)},
	     {ArgType([this](auto &args) {
		   return &get_pair_type(*this, *args.at(0).type, *args.at(1).type);
		 })},			
	     zip_imp);
    
    add_func(*this, "unzip",
	     {ArgType(pair_type)}, {ArgType(0, 0), ArgType(0, 1)},
	     unzip_imp);
    
    add_func(*this, "rfile",
	     {ArgType(path_type)}, {ArgType(rfile_type)},
	     rfile_imp);

    add_func(*this, "rwfile",
	     {ArgType(path_type)}, {ArgType(rwfile_type)},
	     rwfile_imp);

    add_func(*this, "read",
	     {ArgType(readable_type)},
	     {ArgType(get_iter_type(*this, bin_type))},
	     read_imp);

    add_func(*this, "write",
	     {ArgType(writeable_type), ArgType(io_queue_type)},
	     {ArgType(get_iter_type(*this, i64_type))},
	     write_imp);

    add_func(*this, "proc",
	     {ArgType(lambda_type)}, {ArgType(proc_type)},
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

    add_macro(*this, "func:", [this](auto pos, auto &in, auto &out) {
	if (in.size() < 2) {
	  ERROR(Snabel, fmt("Malformed func on row %0, col %1",
			    pos.row, pos.col));
	} else {
	  out.emplace_back(Backup());
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

    add_macro(*this, "proc:", [this](auto pos, auto &in, auto &out) {
	if (in.size() < 2) {
	  ERROR(Snabel, fmt("Malformed proc on row %0, col %1",
			    pos.row, pos.col));
	} else {
	  out.emplace_back(Backup());
	  const str n(in.at(0).text);
	  auto start(std::next(in.begin()));
	  auto end(find_end(start, in.end()));
	  compile(*this, TokSeq(start, end), out);
	  if (end != in.end()) { end++; }
	  in.erase(in.begin(), end);
	  out.emplace_back(Restore());
	  out.emplace_back(Deref("proc"));
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

    add_macro(*this, "call", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Call());
      });

    add_macro(*this, "_", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Drop(1));
      });

    add_macro(*this, "recall", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Recall());
      });

    add_macro(*this, "|", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Reset());
      });

    add_macro(*this, "return", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Return(false));
      });

    add_macro(*this, "$", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Stash());
      });

    add_macro(*this, "when", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Branch());
      });

    add_macro(*this, "yield", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Yield(1));
      });

    add_macro(*this, "yield1", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Yield(2));
      });

    add_macro(*this, "yield2", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Yield(3));
      });

    add_macro(*this, "for", [](auto pos, auto &in, auto &out) {
	out.emplace_back(For());
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
    } else if (&raw == &exe.pair_type) {      
      return get_pair_type(exe, *args.at(0), *args.at(1));
    }

    ERROR(Snabel, fmt("Invalid type: %1", raw.name));
    return raw;
  }

  Type &get_opt_type(Exec &exe, Type &elt) {    
    str n(fmt("Opt<%0>", elt.name));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.opt_type;
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&exe.opt_type);
    t.args.push_back(&elt);
    t.fmt = exe.opt_type.fmt;
    t.dump = exe.opt_type.dump;
    t.eq = exe.opt_type.eq;
    t.equal = exe.opt_type.equal;
    return t;
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
  
  Type &get_list_type(Exec &exe, Type &elt) {    
    str n(fmt("List<%0>", elt.name));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &t(add_type(exe, n));
    t.raw = &exe.list_type;
    t.supers.push_back(&exe.any_type);
    t.supers.push_back(&get_iterable_type(exe, elt));
    t.supers.push_back(&exe.list_type);
    t.args.push_back(&elt);
    t.dump = exe.list_type.dump;
    t.fmt = exe.list_type.fmt;
    t.eq = exe.list_type.eq;
    t.equal = exe.list_type.equal;
    t.iter = exe.list_type.iter;
    return t;
  }

  Type &get_pair_type(Exec &exe, Type &lt, Type &rt) {
    auto &thd(exe.main);
    str n(fmt("Pair<%0 %1>", lt.name, rt.name));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    
    auto &t(add_type(exe, n));
    t.raw = &exe.pair_type;
    t.supers.push_back(&exe.any_type);

    if (isa(thd, lt, exe.ordered_type) && isa(thd, rt, exe.ordered_type)) {
      t.supers.push_back(&exe.ordered_type);
    }

    t.supers.push_back(&exe.pair_type);
    t.args.push_back(&lt);
    t.args.push_back(&rt);

    t.dump = exe.pair_type.dump;
    t.fmt = exe.pair_type.fmt;
    t.eq = exe.pair_type.eq;
    t.equal = exe.pair_type.equal;
    t.lt = exe.pair_type.lt;
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
	       std::forward_as_tuple(exe, tag))
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
      : Box(exe.opt_type, empty_val);
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
    thd.main_scope.coros.clear();
    thd.main_scope.recalls.clear();
    thd.main_scope.return_pc = -1;
    thd.stacks.front().clear();
    thd.pc = 0;
  }

  bool compile(Exec &exe, const str &in) {
    Exec::Lock lock(exe.mutex);
    
    exe.main.ops.clear();
    clear_labels(exe);
    rewind(exe);
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
