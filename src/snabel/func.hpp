#ifndef SNABEL_FUNC_HPP
#define SNABEL_FUNC_HPP

#include <deque>

#include "snackis/core/func.hpp"
#include "snackis/core/opt.hpp"
#include "snackis/core/uid.hpp"

namespace snabel {
  using namespace snackis;

  struct Box;
  struct Coro;
  struct Scope;
  struct Type;
  struct Func;
  
  using Args = std::deque<Box>;

  struct ArgType {
    Type *type;
    opt<size_t> arg_idx, type_arg_idx;

    ArgType(Type &type);
    ArgType(size_t arg_idx, opt<size_t> type_arg_idx=nullopt);
  };
  
  using ArgTypes = std::deque<ArgType>;

  struct FuncImp {
    using Imp = func<void (Scope &, FuncImp &, const Args &)>;

    Func &func;
    ArgTypes args;
    ArgTypes results;
    Imp imp;
    bool pure;
    
    FuncImp(Func &fn,
	    const ArgTypes &args,
	    const ArgTypes &results,
	    Imp imp,
	    bool pure=true);
    void operator ()(Coro &cor, const Args &args);
    void operator ()(Coro &cor);
  };

  struct Func {
    str name;
    std::deque<FuncImp> imps;
    Func(const str &nam);
  };

  Type *get_type(const FuncImp &imp, const ArgType &arg_type, const Args &args);
  Args pop_args(const FuncImp &imp, Coro &cor);
  FuncImp &add_imp(Func &fn,
		   const ArgTypes &args,
		   const ArgTypes &results,
		   FuncImp::Imp imp);
  bool match(const FuncImp &imp, const Coro &cor);
  FuncImp *match(Func &fn, const Coro &cor);
}

#endif
