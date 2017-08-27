#ifndef SNABEL_EXEC_HPP
#define SNABEL_EXEC_HPP

#include <atomic>
#include <mutex>
#include <map>

#include "snabel/label.hpp"
#include "snabel/macro.hpp"
#include "snabel/thread.hpp"
#include "snabel/type.hpp"
#include "snabel/uid.hpp"
#include "snackis/core/uid.hpp"

namespace snabel {
  struct Op;

  const int MAX_TARGET(10);
  
  struct Exec {
    using Lock = std::unique_lock<std::mutex>;
    
    std::map<str, Macro> macros;
    std::map<str, Type> types;
    std::map<std::pair<Type *, Type *>, Conv> convs;
    std::map<str, Func> funcs;
    std::map<str, Label> labels;
    std::deque<Lambda *> lambdas;
    std::map<Thread::Id, Thread> threads;
    
    Thread &main;
    std::mutex mutex;
    Scope &main_scope;
    Type meta_type;
    Type &any_type, &bin_type, &bool_type, &byte_type, &callable_type,
      &char_type, &coro_type, &file_type, &func_type, &i64_type,
      &iter_type,
      &iterable_type, &label_type, &lambda_type, &list_type, &opt_type,
      &ordered_type, &pair_type,
      &path_type, &proc_type, &readable_type, &rfile_type, &random_type, &rat_type,
      &rwfile_type, &str_type, &table_type, &thread_type, &uchar_type, &uid_type,
      &ustr_type, &void_type, &writeable_type;
    Label *return_target[MAX_TARGET], *recall_target[MAX_TARGET],
      *yield_target[MAX_TARGET], *break_target[MAX_TARGET];
    std::atomic<Uid> next_uid;
    
    Exec();
    Exec(const Exec &) = delete;
    const Exec &operator =(const Exec &) = delete;
  };

  Macro &add_macro(Exec &exe, const str &n, Macro::Imp imp);

  Type &get_meta_type(Exec &exe, Type &t);
  Type &add_type(Exec &exe, const str &n);
  Type *find_type(Exec &exe, const str &n);
  Type &get_type(Exec &exe, Type &raw, Types args);
  Type *get_super(Exec &exe, Type &raw, const Types &x, const Types &y);
  Type *get_super(Exec &exe, Type &x, Type &y); 
  Type &get_opt_type(Exec &exe, Type &elt);
  Type &get_iter_type(Exec &exe, Type &elt);
  Type &get_iterable_type(Exec &exe, Type &elt);
  Type &get_list_type(Exec &exe, Type &elt);
  Type &get_pair_type(Exec &exe, Type &lt, Type &rt);

  FuncImp &add_func(Exec &exe,
		    const str n,
		    const ArgTypes &args,
		    const ArgTypes &results,
		    FuncImp::Imp imp);

  Label &add_label(Exec &exe, const str &tag, bool pmt=false);
  Label *find_label(Exec &exe, const str &tag);
  void clear_labels(Exec &exe);
    
  void add_conv(Exec &exe, Type &from, Type &to, Conv conv);
  bool conv(Exec &exe, Box &val, Type &type);

  Uid uid(Exec &exe);
  Box make_opt(Exec &exe, opt<Box> in);

  void rewind(Exec &exe);  
  bool compile(Exec &exe, const str &in);
  bool run(Exec &exe, const str &in);
}

#endif
