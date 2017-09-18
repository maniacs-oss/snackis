#ifndef SNABEL_EXEC_HPP
#define SNABEL_EXEC_HPP

#include <atomic>
#include <deque>
#include <mutex>
#include <map>

#include "snabel/label.hpp"
#include "snabel/macro.hpp"
#include "snabel/thread.hpp"
#include "snabel/sym.hpp"
#include "snabel/type.hpp"
#include "snabel/uid.hpp"
#include "snackis/core/uid.hpp"

namespace snabel {
  struct Op;

  using Convs = std::map<std::pair<Type *, Type *>, Conv>;
  using ConvHandle = Convs::iterator;

  const int MAX_TARGET(10);
  
  struct Exec {
    using Lock = std::unique_lock<std::mutex>;
    
    std::deque<Macro> macros;
    std::deque<Type> types;
    Convs convs;
    std::map<Sym, Func> funcs;
    std::map<Sym, Label> labels;
    std::deque<Begin *> lambdas;
    Threads threads;
    std::map<std::thread::id, Thread *> thread_lookup;
    SymTable syms;
      
    Thread &main;
    std::mutex mutex;
    Scope &main_scope;
    Type meta_type;
    Type &any_type, &bin_type, &bool_type, &byte_type, &callable_type,
      &char_type, &coro_type, &drop_type, &file_type, &func_type, &i64_type,
      &iter_type,
      &iterable_type, &label_type, &lambda_type, &list_type, &macro_type, &nop_type,
      &num_type, &opt_type, &ordered_type, &pair_type,
      &path_type, &quote_type, &readable_type, &rfile_type, &random_type, &rat_type,
      &rwfile_type, &str_type, &struct_type, &sym_type, &table_type, &tcp_server_type,
      &tcp_socket_type, &tcp_stream_type, &thread_type,
      &uchar_type, &uid_type, &ustr_type, &void_type, &wfile_type, &writeable_type;
    Label *return_target[MAX_TARGET], *_return_target[MAX_TARGET],
      *recall_target[MAX_TARGET],
      *yield_target[MAX_TARGET], *_yield_target[MAX_TARGET],
      *break_target[MAX_TARGET];
    std::atomic<Uid> next_uid;
    
    Exec();
    Exec(const Exec &) = delete;
    ~Exec();
    const Exec &operator =(const Exec &) = delete;
  };

  Macro &add_macro(Scope &scp, const str &n, Macro::Imp imp);
  Macro &add_macro(Exec &exe, const str &n, Macro::Imp imp);

  Type &get_meta_type(Exec &exe, Type &t);
  Type &add_type(Exec &exe, const Sym &n, bool meta=false);
  Type &add_type(Exec &exe, const str &n, bool meta=false);
  Type *find_type(Exec &exe, const Sym &n);
  Type &get_type(Exec &exe, Type &raw, Types args);
  Type *get_super(Exec &exe, Type &raw, const Types &x, const Types &y);
  Type &get_opt_type(Exec &exe, Type &elt);
  Type &get_iter_type(Exec &exe, Type &elt);
  Type &get_iterable_type(Exec &exe, Type &elt);
  
  FuncImp &add_func(Exec &exe,
		    const str &n,
		    int sec,
		    const ArgTypes &args,
		    FuncImp::Imp imp);

  Label &add_label(Exec &exe, const Sym &tag, bool pmt=false);
  Label &add_label(Exec &exe, const str &tag, bool pmt=false);
  Label *find_label(Exec &exe, const Sym &tag);
  void clear_labels(Exec &exe);
    
  ConvHandle add_conv(Exec &exe, Type &from, Type &to, Conv conv);
  void rem_conv(Exec &exe, ConvHandle hnd);  
  bool conv(Scope &scp, Box &val, Type &type);

  Thread &curr_thread(Exec &exe);
  Scope &curr_scope(Exec &exe);
  Uid uid(Exec &exe);
  Box make_opt(Exec &exe, opt<Box> in);

  void reset(Exec &exe);  
  void rewind(Exec &exe);  
  bool compile(Exec &exe, TokSeq in, OpSeq &out);
  bool compile(Thread &thd, OpSeq &in);
  bool compile(Exec &exe, OpSeq &in);
  bool compile(Thread &thd, const str &in, bool skip=false);
  bool compile(Exec &exe, const str &in, bool skip=false);
  bool run(Thread &thd, const str &in);
  bool run(Exec &exe, const str &in);

  constexpr Type *get_super(Exec &exe, Type &x, Type &y) {
    if (&x == &y) { return &x; }

    if (x.raw && x.raw == y.raw) {
      return get_super(exe, *x.raw, x.args, y.args);
    }
    
    for (int i(x.supers.size()-1); i >= 0; i--) {
      for (int j(y.supers.size()-1); j >= 0; j--) {
	auto res(get_super(exe, *x.supers[i], *y.supers[j]));
	if (res) { return res; }
      }
    }

    return nullptr;
  }

  template <typename T>
  FuncImp &add_func(Exec &exe,
		    const Sym &n,
		    int sec,
		    const ArgTypes &args,
		    T imp) {
    auto fnd(exe.funcs.find(n));
    auto &scp(curr_scope(exe));

    if (fnd == exe.funcs.end()) {
      auto &fn(exe.funcs.emplace(std::piecewise_construct,
				  std::forward_as_tuple(n),
				  std::forward_as_tuple(n)).first->second);
      put_env(scp, n, Box(scp, exe.func_type, &fn));
      return add_imp(fn, sec, args, imp);
    }

    if (!find_env(scp, n)) {
      put_env(scp, n, Box(scp, exe.func_type, &fnd->second));
    }
    
    return add_imp(fnd->second, sec, args, imp);
  }
}

#endif
