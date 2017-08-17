#ifndef SNABEL_EXEC_HPP
#define SNABEL_EXEC_HPP

#include <atomic>
#include <mutex>
#include <map>

#include "snabel/fiber.hpp"
#include "snabel/label.hpp"
#include "snabel/macro.hpp"
#include "snabel/sym.hpp"
#include "snabel/thread.hpp"
#include "snabel/type.hpp"
#include "snackis/core/uid.hpp"

namespace snabel {
  struct Exec {
    using Lock = std::unique_lock<std::mutex>;
    
    std::map<str, Macro> macros;
    std::map<str, Type> types;
    std::map<std::pair<Type *, Type *>, Conv> convs;
    std::map<str, Func> funcs;
    std::map<str, Label> labels;
    std::deque<str> lambdas;
    std::map<Thread::Id, Thread> threads;
    
    Thread &main_thread;
    std::mutex mutex;
    Fiber &main;
    Scope &main_scope;
    Type meta_type;
    Type &any_type, &bool_type, &callable_type, &char_type, &func_type, &i64_type,
      &iter_type, &iterable_type, &label_type, &lambda_type, &list_type, &pair_type,
      &rat_type, &str_type, &thread_type,
      &undef_type, &void_type;
    std::atomic<Sym> next_gensym;
    
    Exec();
    Exec(const Exec &) = delete;
    const Exec &operator =(const Exec &) = delete;
  };

  Macro &add_macro(Exec &exe, const str &n, Macro::Imp imp);

  Type &get_meta_type(Exec &exe, Type &t);
  Type &add_type(Exec &exe, const str &n);
  Type *find_type(Exec &exe, const str &n);
  Type &get_type(Exec &exe, Type &raw, Types args);
  Type &get_iter_type(Exec &exe, Type &elt);
  Type &get_iterable_type(Exec &exe, Type &elt);
  Type &get_list_type(Exec &exe, Type &elt);
  Type &get_pair_type(Exec &exe, Type &lt, Type &rt);

  FuncImp &add_func(Exec &exe,
		    const str n,
		    const ArgTypes &args,
		    const ArgTypes &results,
		    FuncImp::Imp imp);

  Label &add_label(Exec &exe, const str &tag);
  Label *find_label(Exec &exe, const str &tag);
  void clear_labels(Exec &exe);
    
  void add_conv(Exec &exe, Type &from, Type &to, Conv conv);
  bool conv(Exec &exe, Box &val, Type &type);

  Sym gensym(Exec &exe);
  bool run(Exec &exe, const str &in);
}

#endif
