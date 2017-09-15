#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/io.hpp"
#include "snabel/iter.hpp"
#include "snabel/iters.hpp"
#include "snabel/lambda.hpp"
#include "snabel/list.hpp"
#include "snabel/net.hpp"
#include "snabel/opt.hpp"
#include "snabel/pair.hpp"
#include "snabel/range.hpp"
#include "snabel/rat.hpp"
#include "snabel/str.hpp"
#include "snabel/struct.hpp"
#include "snabel/sym.hpp"
#include "snabel/table.hpp"
#include "snabel/type.hpp"

namespace snabel {
  static void eval_imp(Scope &scp, const Args &args) {
    auto pc(scp.thread.pc);
    compile(scp.exec, *get<StrRef>(args.at(0)), true);
    run(scp.thread);
    scp.thread.pc = pc;
  }

  static void safe_imp(Scope &scp, const Args &args) {
    scp.safe_level++;
  }

  static void is_imp(Scope &scp, const Args &args) {
    auto &v(args.at(0)), &t(args.at(1));
    push(scp, scp.exec.bool_type, isa(scp.thread, v, *get<Type *>(t)));
  }

  static void type_imp(Scope &scp, const Args &args) {
    auto &v(args.at(0));
    push(scp, get_meta_type(scp.exec, *v.type), v.type);
  }

  static void eq_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp, scp.exec.bool_type, x.type->eq(x, y));
  }

  static void equal_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp, scp.exec.bool_type, x.type->equal(x, y));
  }

  static void lt_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp, scp.exec.bool_type, x.type->lt(x, y));
  }

  static void lte_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp, scp.exec.bool_type,
	 x.type->lt(x, y) ||
	 x.type->eq(x, y));
  }

  static void gt_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp, scp.exec.bool_type, x.type->gt(x, y));
  }

  static void gte_imp(Scope &scp, const Args &args) {
    auto &x(args.at(0)), &y(args.at(1));
    push(scp, scp.exec.bool_type,
	 x.type->gt(x, y) ||
	 x.type->eq(x, y));
  }

  static void when_imp(Scope &scp, const Args &args) {
    auto &cnd(args.at(0)), &tgt(args.at(1));
    if (get<bool>(cnd)) { tgt.type->call(scp, tgt, false); }
  }

  static void unless_imp(Scope &scp, const Args &args) {
    auto &cnd(args.at(0)), &tgt(args.at(1));
    if (!get<bool>(cnd)) { tgt.type->call(scp, tgt, false); }
  }

  static void if_imp(Scope &scp, const Args &args) {
    auto &cnd(get<bool>(args.at(0)));
    auto &tgt(args.at(cnd ? 1 : 2));
    tgt.type->call(scp, tgt, false);
  }

  static void cond_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto in(args.at(0));
    auto it((*in.type->iter)(in));

    while (true) {
      auto nxt(it->next(scp));
      if (!nxt) { break; }
      auto &cs(*get<PairRef>(*nxt));
      cs.first.type->call(scp, cs.first, true);
      auto res(try_pop(scp.thread));
      if (!res) { continue; }
      
      if (res->type != &exe.bool_type) {
	ERROR(Snabel, fmt("Invalid cond case result: %0", *res));
	break;
      }

      if (get<bool>(*res)) {
	cs.second.type->call(scp, cs.second, true);
	break;
      }
    }
  }
  
  static void zero_i64_imp(Scope &scp, const Args &args) {
    bool res(get<int64_t>(args.at(0)) == 0);
    push(scp, scp.exec.bool_type, res);
  }

  static void pos_i64_imp(Scope &scp, const Args &args) {
    bool res(get<int64_t>(args.at(0)) > 0);
    push(scp, scp.exec.bool_type, res);
  }

  static void inc_i64_imp(Scope &scp, const Args &args) {
    auto &in(get<int64_t>(args.at(0)));
    push(scp, scp.exec.i64_type, in+1);
  }

  static void dec_i64_imp(Scope &scp, const Args &args) {
    auto &in(get<int64_t>(args.at(0)));
    push(scp, scp.exec.i64_type, in-1);
  }

  static void add_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp, scp.exec.i64_type, x+y);
  }

  static void sub_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp, scp.exec.i64_type, x-y);
  }

  static void mul_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp, scp.exec.i64_type, x*y);
  }

  static void mod_i64_imp(Scope &scp, const Args &args) {
    auto &x(get<int64_t>(args.at(0))), &y(get<int64_t>(args.at(1)));
    push(scp, scp.exec.i64_type, x%y);
  }

  static void trunc_imp(Scope &scp, const Args &args) {
    push(scp, scp.exec.i64_type, trunc(get<Rat>(args.at(0))));
  }

  static void bin_zero_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp, scp.exec.bool_type, get<BinRef>(in)->empty());
  }

  static void bin_pos_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp, scp.exec.bool_type, !get<BinRef>(in)->empty());
  }

  static void bin_len_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    push(scp, scp.exec.i64_type, (int64_t)get<BinRef>(in)->size());
  } 

  static void bin_append_imp(Scope &scp, const Args &args) {
    auto &out(args.at(0));
    auto &tgt(*get<BinRef>(out));
    auto &src(*get<BinRef>(args.at(1)));
    std::copy(src.begin(), src.end(), std::back_inserter(tgt));
    push(scp.thread, out);
  }

  static void iter_imp(Scope &scp, const Args &args) {
    auto &in(args.at(0));
    auto it((*in.type->iter)(in));
    push(scp, it->type, it);
  }
  
  static void iterable_filter_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0)), &tgt(args.at(1));
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0));
    
    push(scp,
	 get_iter_type(exe, elt),
	 IterRef(new FilterIter(exe, it, elt, tgt)));
  }

  static void iterable_map_imp(Scope &scp, const Args &args) {
    auto &exe(scp.exec);
    auto &in(args.at(0)), &tgt(args.at(1));
    auto it((*in.type->iter)(in));
    auto &elt(*it->type.args.at(0));
    
    push(scp,
	 get_iter_type(exe, elt),
	 IterRef(new MapIter(exe, it, tgt)));
  }
  
  static void bytes_imp(Scope &scp, const Args &args) {
    push(scp,
	 scp.exec.bin_type,
	 std::make_shared<Bin>(get<int64_t>(args.at(0))));
  }

  static void uid_imp(Scope &scp, const Args &args) {
    push(scp, scp.exec.uid_type, uid(scp.exec)); 
  }  

  static void random_imp(Scope &scp, const Args &args) {
    push(scp,
	 scp.exec.random_type,
	 std::make_shared<Random>(0, get<int64_t>(args.at(0))));
  }

  static void random_pop_imp(Scope &scp, const Args &args) {
    auto &r(args.at(0));
    push(scp,
	 scp.exec.i64_type,
	 (*get<RandomRef>(r))(scp.thread.random));
  }

  static str meta_fmt(const Box &v) {
    return fmt("%0", name(get<Type *>(v)->name));
  }

  static bool meta_eq(const Box &x, const Box &y) {
    return get<Type *>(x) == get<Type *>(y);
  }
  
  Exec::Exec():
    main(threads.emplace(std::piecewise_construct,
				std::forward_as_tuple(0),
				std::forward_as_tuple(*this, 0)).first->second),
    main_scope(main.scopes.at(0)),
    meta_type(get_sym(*this, "Type<Any>")),
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
    macro_type(add_type(*this, "Macro")),
    nop_type(add_type(*this, "Nop")),
    opt_type(add_type(*this, "Opt")),
    ordered_type(add_type(*this, "Ordered")),
    pair_type(add_type(*this, "Pair")),
    path_type(add_type(*this, "Path")),
    quote_type(add_type(*this, "Quote")),
    readable_type(add_type(*this, "Readable")),
    rfile_type(add_type(*this, "RFile")),
    random_type(add_type(*this, "Random")),
    rat_type(add_type(*this, "Rat")),
    rwfile_type(add_type(*this, "RWFile")),    
    str_type(add_type(*this, "Str")),
    struct_type(add_type(*this, "Struct")),
    sym_type(add_type(*this, "Sym")),
    table_type(add_type(*this, "Table")),
    tcp_server_type(add_type(*this, "TCPServer")),
    tcp_socket_type(add_type(*this, "TCPSocket")),
    tcp_stream_type(add_type(*this, "TCPStream")),
    thread_type(add_type(*this, "Thread")),
    uchar_type(add_type(*this, "UChar")),
    uid_type(add_type(*this, "Uid")),
    ustr_type(add_type(*this, "UStr")),
    void_type(add_type(*this, "Void")),
    wfile_type(add_type(*this, "WFile")),
    writeable_type(add_type(*this, "Writeable")),
    return_target {
    &add_label(*this, "_return", true), &add_label(*this, "_return1", true),
      &add_label(*this, "_return2", true), &add_label(*this, "_return3", true),
      &add_label(*this, "_return4", true), &add_label(*this, "_return5", true),
      &add_label(*this, "_return6", true), &add_label(*this, "_return7", true),
      &add_label(*this, "_return8", true), &add_label(*this, "_return9", true)},
    _return_target {
    &add_label(*this, "__return", true), &add_label(*this, "__return1", true),
      &add_label(*this, "__return2", true), &add_label(*this, "__return3", true),
      &add_label(*this, "__return4", true), &add_label(*this, "__return5", true),
      &add_label(*this, "__return6", true), &add_label(*this, "__return7", true),
      &add_label(*this, "__return8", true), &add_label(*this, "__return9", true)},
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
    _yield_target {
    &add_label(*this, "__yield", true), &add_label(*this, "__yield1", true),
      &add_label(*this, "__yield2", true), &add_label(*this, "__yield3", true),
      &add_label(*this, "__yield4", true), &add_label(*this, "__yield5", true),
      &add_label(*this, "__yield6", true), &add_label(*this, "__yield7", true),
      &add_label(*this, "__yield8", true), &add_label(*this, "__yield9", true)},
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
    meta_type.fmt = meta_fmt;
    meta_type.eq = meta_eq;

    void_type.fmt = [](auto &v) { return "Void"; };
    void_type.eq = [](auto &x, auto &y) { return true; };  
    
    ordered_type.supers.push_back(&any_type);

    callable_type.supers.push_back(&any_type);
    callable_type.args.push_back(&any_type);

    nop_type.supers.push_back(&callable_type);
    nop_type.fmt = [](auto &v) { return "&nop"; };
    nop_type.eq = [](auto &x, auto &y) { return true; };  
    nop_type.call = [](auto &scp, auto &v, bool now) { return true; };
    
    drop_type.supers.push_back(&callable_type);
    drop_type.fmt = [](auto &v) { return "&_"; };
    drop_type.eq = [](auto &x, auto &y) { return true; };  

    drop_type.call = [](auto &scp, auto &v, bool now) {
      return try_pop(scp.thread) ? true : false;
    };
    
    iter_type.supers.push_back(&any_type);
    iter_type.args.push_back(&any_type);

    iter_type.fmt = [](auto &v) {
      return fmt("Iter<%0>", name(v.type->args.at(0)->name));
    };
    
    iter_type.eq = [](auto &x, auto &y) {
      return get<IterRef>(x) == get<IterRef>(y);
    };
    
    iter_type.iter = [](auto &in) { return get<IterRef>(in); };

    iterable_type.supers.push_back(&any_type);
    iterable_type.args.push_back(&any_type);

    iterable_type.fmt = [](auto &v) {
      return fmt("Iterable<%0>", name(v.type->args.at(0)->name));
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
    bool_type.fmt = [](auto &v) { return get<bool>(v) ? "true" : "false"; };
    bool_type.eq = [](auto &x, auto &y) { return get<bool>(x) == get<bool>(y); };
    put_env(main_scope, "true", Box(main_scope, bool_type, true));
    put_env(main_scope, "false", Box(main_scope, bool_type, false));

    macro_type.supers.push_back(&any_type);

    macro_type.fmt = [](auto &v) {
      return fmt("Macro(%0)", name(get<Macro *>(v)->name));
    };

    macro_type.eq = [](auto &x, auto &y) {
      return get<Macro *>(x) == get<Macro *>(y);
    };

    func_type.supers.push_back(&any_type);
    func_type.supers.push_back(&callable_type);

    func_type.fmt = [](auto &v) {
      return fmt("Func(%0)", name(get<Func *>(v)->name));
    };

    func_type.eq = [](auto &x, auto &y) { return get<Func *>(x) == get<Func *>(y); };
    
    func_type.call = [](auto &scp, auto &v, bool now) {
      TRY(try_call);
      auto &thd(scp.thread);
      auto &fn(*get<Func *>(v));
      auto m(match(fn, scp));
      
      if (!m) {
	ERROR(Snabel, fmt("Function not applicable: %0\n%1", 
			  name(fn.name), curr_stack(thd)));
	return false;
      }

      if (!try_call.errors.empty()) { return false; }
      (*m->first)(scp, m->second);
      return true;
    };

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
      return fmt("%0:%1", name(l.tag), l.pc);
    };

    label_type.eq = [](auto &x, auto &y) {
      return get<Label *>(x) == get<Label *>(y);
    };

    label_type.call = [](auto &scp, auto &v, bool now) {
      jump(scp, *get<Label *>(v));
      return true;
    };

    lambda_type.supers.push_back(&callable_type);

    lambda_type.fmt = [](auto &v) {
      auto &l(get<LambdaRef>(v)->label);
      return fmt("Lambda(%0:%1)", name(l.tag), l.pc);
    };
    
    lambda_type.eq = [](auto &x, auto &y) {
      return get<LambdaRef>(x) == get<LambdaRef>(y);
    };

    lambda_type.call = [](auto &scp, auto &v, bool now) {
      call(get<LambdaRef>(v), scp, now);
      return true;
    };
        
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

    for (int i(0); i < MAX_TARGET; i++) {
      return_target[i]->return_depth = i+1;
      _return_target[i]->return_depth = i+1;
      _return_target[i]->push_result = false;
      recall_target[i]->recall_depth = i+1;
      yield_target[i]->yield_depth = i+1;
      _yield_target[i]->yield_depth = i+1;
      _yield_target[i]->push_result = false;
      break_target[i]->break_depth = i+1;
    }

    init_types(*this);
    init_opts(*this);
    init_strs(*this);
    init_syms(*this);
    init_pairs(*this);
    init_rats(*this);
    init_lists(*this);
    init_tables(*this);
    init_structs(*this);
    init_coros(*this);
    init_io(*this);
    init_net(*this);
    init_threads(*this);
        
    add_func(*this, "eval", Func::Safe, {ArgType(str_type)}, eval_imp);
    add_func(*this, "safe", Func::Safe, {}, safe_imp);
    
    add_func(*this, "is?", Func::Pure,
	     {ArgType(any_type), ArgType(meta_type)},
	     is_imp);
    add_func(*this, "type", Func::Pure, {ArgType(any_type)}, type_imp);

    add_func(*this, "=", Func::Pure, {ArgType(any_type), ArgType(0)}, eq_imp);
    add_func(*this, "==", Func::Pure, {ArgType(any_type), ArgType(0)}, equal_imp);
    
    add_func(*this, "lt?", Func::Pure, {ArgType(ordered_type), ArgType(0)}, lt_imp);
    add_func(*this, "lte?", Func::Pure, {ArgType(ordered_type), ArgType(0)}, lte_imp);
    add_func(*this, "gt?", Func::Pure, {ArgType(ordered_type), ArgType(0)}, gt_imp);
    add_func(*this, "gte?", Func::Pure, {ArgType(ordered_type), ArgType(0)}, gte_imp);

    add_func(*this, "when", Func::Pure,
	     {ArgType(bool_type), ArgType(any_type)},
	     when_imp);

    add_func(*this, "unless", Func::Pure,
	     {ArgType(bool_type), ArgType(any_type)},
	     unless_imp);

    add_func(*this, "if", Func::Pure,
	     {ArgType(bool_type), ArgType(any_type), ArgType(any_type)},
	     if_imp);

    add_func(*this, "cond", Func::Pure,
	     {ArgType(get_iterable_type(*this, pair_type))},
	     cond_imp);

    add_func(*this, "z?", Func::Pure, {ArgType(i64_type)}, zero_i64_imp);
    add_func(*this, "+?", Func::Pure, {ArgType(i64_type)}, pos_i64_imp);
    add_func(*this, "++", Func::Pure, {ArgType(i64_type)}, inc_i64_imp);
    add_func(*this, "--", Func::Pure, {ArgType(i64_type)}, dec_i64_imp);

    add_func(*this, "+", Func::Pure,
	     {ArgType(i64_type), ArgType(i64_type)},
	     add_i64_imp);
    
    add_func(*this, "-", Func::Pure,
	     {ArgType(i64_type), ArgType(i64_type)},
	     sub_i64_imp);

    add_func(*this, "*", Func::Pure,
	     {ArgType(i64_type), ArgType(i64_type)},
	     mul_i64_imp);
        
    add_func(*this, "%", Func::Pure,
	     {ArgType(i64_type), ArgType(i64_type)},
	     mod_i64_imp);

    add_func(*this, "trunc", Func::Pure, {ArgType(rat_type)}, trunc_imp);

    add_func(*this, "bytes", Func::Pure, {ArgType(i64_type)}, bytes_imp);
    add_func(*this, "z?", Func::Pure, {ArgType(bin_type)}, bin_zero_imp);
    add_func(*this, "+?", Func::Pure, {ArgType(bin_type)}, bin_pos_imp);
    add_func(*this, "len", Func::Pure, {ArgType(bin_type)}, bin_len_imp);
    add_func(*this, "append", Func::Safe,
	     {ArgType(bin_type), ArgType(bin_type)},
	     bin_append_imp);

    add_func(*this, "uid", Func::Const, {}, uid_imp);

    add_func(*this, "iter", Func::Const, {ArgType(iterable_type)}, iter_imp);

    add_func(*this, "filter", Func::Const, 
	     {ArgType(iterable_type), ArgType(callable_type)},
	     iterable_filter_imp);

    add_func(*this, "map", Func::Const,
	     {ArgType(iterable_type), ArgType(callable_type)},
	     iterable_map_imp);
        
    add_func(*this, "random", Func::Const, {ArgType(i64_type)}, random_imp);
    add_func(*this, "pop", Func::Safe, {ArgType(random_type)}, random_pop_imp);

    add_macro(*this, "macro:", [this](auto pos, auto &in, auto &out) {
	if (in.empty()) {
	  ERROR(Snabel, fmt("Malformed macro definition on row %0, col %1",
			    pos.row, pos.col));
	  return;
	} else {
	  auto &n(in.at(0).text);
	  in.pop_front();
	  auto end(find_end(in.begin(), in.end()));
	  OpSeq imp;
	  imp.emplace_back(Begin(*this));
	  compile(*this, TokSeq(in.begin(), end), imp);
	  in.erase(in.begin(), std::next(end));
	  imp.emplace_back(End(*this));
	  if (!compile(*this, imp)) { return; }
	  run(main);
	  auto lmb(try_pop(main));

	  if (!lmb) {
	    ERROR(Snabel, "Missing macro lambda");
	    return;
	  }
	  
	  add_macro(*this, n, get<LambdaRef>(*lmb));
	}
      });

    add_macro(*this, "`", [this](auto pos, auto &in, auto &out) {
	static const std::map<str, str> delims {
	  std::make_pair("(", ")"),
	    std::make_pair("{", "}"),
	    std::make_pair("[", "]") };

	OutStream buf;
	auto &tok(in.front());
	buf << tok.text;
	in.pop_front();
	auto fnd(delims.find(tok.text));
	
	if (fnd != delims.end()) {
	  str beg(fnd->first), end(fnd->second);
	  int depth(1);
	  str sep("");
	  
	  while (!in.empty() && depth) {
	    auto &tok(in.front());
	    
	    if (tok.text == beg) {
	      depth++;
	    } else if (tok.text == end) {
	      depth--;
	    }
 
	    buf << sep << tok.text;
	    in.pop_front();
	    sep = " ";
	  }

	  if (depth) {
	    ERROR(Snabel, fmt("Missing %0 at row %1, col %2", end, pos.row, pos.col));
	    return;
	  }
	}

	out.emplace_back(Push(Box(main_scope,
				  quote_type,
				  std::make_shared<str>(buf.str()))));
      });
    
    add_macro(*this, "{", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Begin(*this));
      });

    add_macro(*this, "}", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(End(*this));
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
	  out.emplace_back(Begin(*this));
	  const str n(in.at(0).text);
	  auto start(std::next(in.begin()));
	  auto end(find_end(start, in.end()));
	  compile(*this, TokSeq(start, end), out);
	  if (end != in.end()) { end++; }
	  in.erase(in.begin(), end);
	  out.emplace_back(End(*this));
	  out.emplace_back(Putenv(get_sym(*this, n)));
	}
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
	  out.emplace_back(Putenv(get_sym(*this, fmt("@%0", n))));
	}
      });
    
    add_macro(*this, "$list", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Stash());
      });

    add_macro(*this, "recall", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Recall(1));
      });

    add_macro(*this, "return", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Return(1, true));
      });

    add_macro(*this, "_return", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Return(1, false));
      });

    add_macro(*this, "yield", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Yield(1, true));
      });

    add_macro(*this, "_yield", [this](auto pos, auto &in, auto &out) {
	out.emplace_back(Yield(1, false));
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

    add_macro(*this, "loop", [](auto pos, auto &in, auto &out) {
	out.emplace_back(Loop());
      });
  }

  Exec::~Exec() {
    for (auto &s: syms) { delete s.second.pos; }
  }
  
  Macro &add_macro(Scope &scp, const str &n, Macro::Imp imp) {
    auto &exe(scp.exec);
    auto &ns(get_sym(exe, n));
    auto &m(exe.macros.emplace_back(ns, imp));
    put_env(scp, ns, Box(scp, exe.macro_type, &m));
    return m;
  }

  Macro &add_macro(Exec &exe, const str &n, Macro::Imp imp) {
    return add_macro(curr_scope(exe.main), n, imp);
  }
  
  Macro &add_macro(Scope &scp, const str &n, const LambdaRef &lmb) {
    auto &exe(scp.exec);
    auto &ns(get_sym(exe, n));
    auto &m(exe.macros.emplace_back(exe, ns, lmb)); 
    put_env(scp, ns, Box(scp, exe.macro_type, &m));
    return m;
  }

  Macro &add_macro(Exec &exe, const str &n, const LambdaRef &lmb) {
    return add_macro(curr_scope(exe.main), n, lmb);
  }

  Type &get_meta_type(Exec &exe, Type &t) {    
    auto &n(get_sym(exe, fmt("Type<%0>", name(t.name))));
    auto fnd(find_type(exe, n));
    if (fnd) { return *fnd; }
    auto &mt(add_type(exe, n, true));
    mt.raw = &exe.meta_type;
    mt.supers.push_back(&exe.meta_type);
    mt.args.push_back(&t);
    mt.fmt = meta_fmt;
    mt.eq = meta_eq;
    return mt;
  }

  Type &add_type(Exec &exe, const Sym &n, bool meta) {
    auto fnd(find_env(exe.main_scope, n));
    auto &t(exe.types.emplace_back(n));
    auto &mt(meta ? exe.meta_type : get_meta_type(exe, t));
    
    if (fnd) {
      ERROR(Redefine, fmt("Redefining type: %0", name(n)));
      fnd->type = &mt;
      fnd->val = &t;
    } else {
      put_env(exe.main_scope, n, Box(exe.main_scope, mt, &t));
    }
    
    return t;
  }

  Type &add_type(Exec &exe, const str &n, bool meta) {
    return add_type(exe, get_sym(exe, n), meta);
  }

  Type *find_type(Exec &exe, const Sym &n) {
    auto fnd(find_env(exe.main_scope, n));
    if (!fnd) { return nullptr; }
    return get<Type *>(*fnd);
  }

  Type &get_type(Exec &exe, Type &raw, Types args) {
    if (args.empty()) { return raw; }
    
    if (args.size() > raw.args.size()) {
      ERROR(Snabel, fmt("Too many params for type %0", name(raw.name)));
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

    ERROR(Snabel, fmt("Invalid type: %1", name(raw.name)));
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

  Type &get_iter_type(Exec &exe, Type &elt) {    
    auto &n(get_sym(exe, fmt("Iter<%0>", name(elt.name))));
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
    auto &n(get_sym(exe, fmt("Iterable<%0>", name(elt.name))));
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
		    const Sym &n,
		    int sec,
		    const ArgTypes &args,
		    FuncImp::Imp imp) {
    auto fnd(exe.funcs.find(n));

    if (fnd == exe.funcs.end()) {
      auto &fn(exe.funcs.emplace(std::piecewise_construct,
				  std::forward_as_tuple(n),
				  std::forward_as_tuple(n)).first->second);
      put_env(exe.main_scope, n, Box(exe.main_scope, exe.func_type, &fn));
      return add_imp(fn, sec, args, imp);
    }
    
    return add_imp(fnd->second, sec, args, imp);
  }

  FuncImp &add_func(Exec &exe,
		    const str &n,
		    int sec,
		    const ArgTypes &args,
		    FuncImp::Imp imp) {
    return add_func(exe, get_sym(exe, n), sec, args, imp);
  }
  
  Label &add_label(Exec &exe, const Sym &tag, bool pmt) {
    auto res(exe.labels
	     .emplace(std::piecewise_construct,
		      std::forward_as_tuple(tag),
		      std::forward_as_tuple(exe, tag, pmt)));

    if (!res.second) {
      ERROR(Snabel, fmt("Duplicate label: %0", name(tag)));
    }

    return res.first->second;
  }

  Label &add_label(Exec &exe, const str &tag, bool pmt) {
    return add_label(exe, get_sym(exe, tag), pmt);
  }

  void clear_labels(Exec &exe) {
    for (auto i(exe.labels.begin()); i != exe.labels.end();) {
      auto &l(i->second);

      if (l.permanent) {
	i++;
      } else {
	i = exe.labels.erase(i);
      }
    }
  }

  Label *find_label(Exec &exe, const Sym &tag) {
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
      ? Box(exe.main_scope, get_opt_type(exe, *in->type), in->val)
      : Box(exe.main_scope, exe.any_type);
  }

  void reset(Exec &exe) {
    clear_labels(exe);
    exe.next_uid.store(1);
    exe.main.ops.clear();
    exe.main.pc = 0;
    rewind(exe);
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
    thd.main.return_pc = -1;
    thd.stacks.front().clear();
  }

  bool compile(Exec &exe, TokSeq in, OpSeq &out) {
    TRY(try_compile);

    while (!in.empty()) {
     Tok tok(in.at(0));
      in.pop_front();
    
      if (tok.text.size() > 1 &&
	  tok.text.at(0) == '/' &&
	  (tok.text.at(1) == '*' || tok.text.at(1) == '/')) {
	// Skip comments
      } else if (tok.text.at(0) == '#') {
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.sym_type,
				  get_sym(exe, tok.text.substr(1)))));
      } else if (tok.text == "&_") {
	out.emplace_back(Push(Box(exe.main_scope, exe.drop_type, nullptr)));
      } else if (tok.text == "&nop") {
	out.emplace_back(Push(Box(exe.main_scope, exe.nop_type, nullptr)));    
      } else if (tok.text.substr(0, 6) == "return" &&
		 tok.text.size() == 7 &&
		 isdigit(tok.text.at(6))) {
	auto i(tok.text.at(6) - '0');
	out.emplace_back(Return(i+1, true));
      } else if (tok.text.substr(0, 7) == "_return" &&
		 tok.text.size() == 8 &&
		 isdigit(tok.text.at(7))) {
	auto i(tok.text.at(7) - '0');
	out.emplace_back(Return(i+1, false));
      } else if (tok.text == "&return") {
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.return_target[0])));
      } else if (tok.text == "&_return") {
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe._return_target[0])));
      } else if (tok.text.substr(0, 7) == "&return" &&
		 tok.text.size() == 8 &&
		 isdigit(tok.text.at(7))) {
	auto i(tok.text.at(7) - '0');
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.return_target[i])));
      } else if (tok.text.substr(0, 8) == "&_return" &&
		 tok.text.size() == 9 &&
		 isdigit(tok.text.at(8))) {
	auto i(tok.text.at(8) - '0');
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe._return_target[i])));
      } else if (tok.text.substr(0, 6) == "recall" &&
		 tok.text.size() == 7 &&
		 isdigit(tok.text.at(6))) {
	auto i(tok.text.at(6) - '0');
	out.emplace_back(Recall(i+1));
      } else if (tok.text == "&recall") {
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.recall_target[0])));
      } else if (tok.text.substr(0, 7) == "&recall" &&
		 tok.text.size() == 8 &&
		 isdigit(tok.text.at(7))) {
	auto i(tok.text.at(7) - '0');
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.recall_target[i])));
      } else if (tok.text.substr(0, 5) == "yield" &&
		 tok.text.size() == 6 &&
		 isdigit(tok.text.at(5))) {
	auto i(tok.text.at(5) - '0');
	out.emplace_back(Yield(i+1, true));
      } else if (tok.text.substr(0, 6) == "_yield" &&
		 tok.text.size() == 7 &&
		 isdigit(tok.text.at(6))) {
	auto i(tok.text.at(6) - '0');
	out.emplace_back(Yield(i+1, false));
      } else if (tok.text == "&yield") {
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.yield_target[0])));
      } else if (tok.text == "&_yield") {
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe._yield_target[0])));
      } else if (tok.text.substr(0, 6) == "&yield" &&
		 tok.text.size() == 7 &&
		 isdigit(tok.text.at(6))) {
	auto i(tok.text.at(6) - '0');
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.yield_target[i])));
      } else if (tok.text.substr(0, 7) == "&_yield" &&
		 tok.text.size() == 8 &&
		 isdigit(tok.text.at(7))) {
	auto i(tok.text.at(7) - '0');
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe._yield_target[i])));
      } else if (tok.text.substr(0, 5) == "break" &&
		 tok.text.size() == 6 &&
		 isdigit(tok.text.at(5))) {
	auto i(tok.text.at(5) - '0');
	out.emplace_back(Break(i+1));
      } else if (tok.text == "&break") {
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.break_target[0])));
      } else if (tok.text.substr(0, 6) == "&break" &&
		 tok.text.size() == 7 &&
		 isdigit(tok.text.at(6))) {
	auto i(tok.text.at(6) - '0');
	out.emplace_back(Push(Box(exe.main_scope,
				  exe.label_type,
				  exe.break_target[i])));
      } else if (tok.text.at(0) == '&') {
	out.emplace_back(Getenv(get_sym(exe, tok.text.substr(1))));
      } else if (tok.text.at(0) == '@') {
	out.emplace_back(Getenv(get_sym(exe, tok.text)));
      } else if (tok.text.at(0) == '$' &&
		 tok.text.size() == 2 &&
		 isdigit(tok.text.at(1))) {
	auto i(tok.text.at(1) - '0');
	if (i) {
	  out.emplace_back(Swap(i));
	} else {
	  out.emplace_back(Dup());
	}
      } else if (tok.text.at(0) == '\'') {
	auto s(tok.text.substr(1, tok.text.size()-2));
	replace<str>(s, "\\n", "\n");
	replace<str>(s, "\\r", "\r");
	replace<str>(s, "\\t", "\t");
	replace<str>(s, "\\'", "'");
	bool fmt(false);
      
	for (size_t i(0); i < s.size(); i++) {
	  auto &c(s[i]);
	
	  if ((c == '$' || c == '@')  && (!i || s[i-1] != '\\')) {
	    fmt = true;
	  }
	}

	if (fmt) {
	  out.emplace_back(Fmt(s));
	} else {
	  out.emplace_back(Push(Box(exe.main_scope,
				    exe.str_type,
				    std::make_shared<str>(s))));
	}
      } else if (tok.text.at(0) == '"') {
	auto s(uconv.from_bytes(tok.text.substr(1, tok.text.size()-2)));
	replace<ustr>(s, u"\\n", u"\n");
	replace<ustr>(s, u"\\r", u"\r");
	replace<ustr>(s, u"\\t", u"\t");
	replace<ustr>(s, u"\\\"", u"\"");

	out.emplace_back(Push(Box(exe.main_scope,
				  exe.ustr_type,
				  std::make_shared<ustr>(s))));
      } else if (tok.text.at(0) == '\\' && tok.text.at(1) == '\\') {
	if (tok.text.size() < 3) {
	  ERROR(Snabel, fmt("Invalid uchar literal on row %0, col %1: %2",
			    tok.pos.row, tok.pos.col, tok.text));
	  break;
	}

	uchar c(0);

	if (tok.text == "\\\\space") {
	  c = u' ';
	} else if (tok.text == "\\\\n") {
	  c = u'\n';
	} else if (tok.text == "\\\\t") {
	  c = u'\t';
	} else {
	  c = uconv.from_bytes(str(1, tok.text[2])).at(0);
	}
      
	out.emplace_back(Push(Box(exe.main_scope, exe.uchar_type, c)));
      } else if (tok.text.at(0) == '\\') {
	if (tok.text.size() < 2) {
	  ERROR(Snabel, fmt("Invalid char literal on row %0, col %1: %2",
			    tok.pos.row, tok.pos.col, tok.text));
	  break;
	}

	char c(0);

	if (tok.text == "\\space") {
	  c = ' ';
	} else if (tok.text == "\\n") {
	  c = '\n';
	} else if (tok.text == "\\t") {
	  c = '\t';
	} else {
	  c = tok.text[1];
	}
      
	out.emplace_back(Push(Box(exe.main_scope, exe.char_type, c)));
      } else if (isupper(tok.text[0])) {
	auto t(parse_type(exe, tok.text, 0).first);

	if (t) {
	  out.emplace_back(Push(Box(exe.main_scope,
				    get_meta_type(exe, *t),
				    t)));
	}
      } else if (isdigit(tok.text[0]) || 
		 (tok.text.size() > 1 &&
		  tok.text[0] == '-' &&
		  isdigit(tok.text[1]))) {
	out.emplace_back(Push(Box(exe.main_scope, exe.i64_type, to_int64(tok.text))));
      } else {
	auto ts(get_sym(exe, tok.text));
	auto fnd(find_env(curr_scope(exe.main), ts));
      
	if (fnd && fnd->type == &exe.macro_type) {
	  (*get<Macro *>(*fnd))(tok.pos, in, out);
	} else {
	  out.emplace_back(Deref(ts));
	}
      }

      for (auto e: try_compile.errors) {
	if (!dynamic_cast<RedefineError *>(e)) { return false; }
      }
    }

    return true;
  }

  bool compile(Exec &exe, const str &in, bool skip) {
    Exec::Lock lock(exe.mutex);

    TokSeq toks;
    parse_expr(in, 0, toks);
    Label *skip_label(nullptr);
    OpSeq in_ops;

    if (skip) {
      skip_label = &add_label(exe, fmt("_skip%0", uid(exe)));
      in_ops.emplace_back(Jump(*skip_label));
    }

    if (!compile(exe, toks, in_ops)) { return false; }
    if (skip) { in_ops.emplace_back(Target(*skip_label)); }
    if (!compile(exe, in_ops)) { return false; }
    if (skip) { exe.main.pc++; }
    return true;
  }

  bool compile(Exec &exe, OpSeq &in) {
    auto start_pc(exe.main.ops.size());
    TRY(try_compile);
    
    while (true) {
      exe.main.pc = start_pc;
      
      for (auto &op: in) {
	if (!op.prepared && !prepare(op, exe.main_scope)) { goto exit; }
	exe.main.pc++;
      }

      exe.main.pc = start_pc;
      bool done(false);
      
      while (!done) {
	done = true;
	
	for (auto &op: in) {
	  if (refresh(op, curr_scope(exe.main))) { done = false; }
	  exe.main.pc++;
	}
      }

      OpSeq out_ops;
      done = true;

      for (auto &op: in) {
	if (compile(op, curr_scope(exe.main), out_ops)) { done = false; }
      }

      if (done) { break; }      
      in.clear();
      in.swap(out_ops);
      try_compile.errors.clear();
    }

    {
      OpSeq out_ops;
      exe.main.pc = start_pc;
      
      for (auto &op: in) {
	if (!finalize(op, exe.main_scope, out_ops)) {
	  exe.main.pc++;
	}
	
	for (auto e: try_compile.errors) {
	  if (!dynamic_cast<RedefineError *>(e)) { goto exit; }
	}
      }
      
      in.clear();
      in.swap(out_ops);
    }
  exit:
    exe.main.pc = start_pc;
    std::copy(in.begin(), in.end(), std::back_inserter(exe.main.ops));

    for (auto e: try_compile.errors) {
      if (!dynamic_cast<RedefineError *>(e)) { return false; }
    }
    
    return true;
  }
  
  bool run(Exec &exe, const str &in) {
    return compile(exe, in) && run(exe.main);
  }
}
