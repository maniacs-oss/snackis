#ifndef SNABEL_FUNC_HPP
#define SNABEL_FUNC_HPP

#include <deque>

#include "snackis/core/func.hpp"
#include "snackis/core/opt.hpp"
#include "snackis/core/uid.hpp"

namespace snabel {
  using namespace snackis;

  struct Box;
  struct Func;
  struct Scope;
  struct Thread;
  struct Type;
  
  using Args = std::deque<Box>;

  struct ArgType {
    using Fn = func<Type *(const Args &)>;

    Type *type;
    opt<size_t> arg_idx, type_arg_idx;
    opt<Fn> fn;
    
    ArgType(Type &type);
    ArgType(size_t arg_idx);
    ArgType(size_t arg_idx, size_t type_arg_idx);
    ArgType(Fn fn);
  };
  
  using ArgTypes = std::deque<ArgType>;

  struct FuncImp {
    using Imp = func<void (Scope &, const Args &)>;

    Func &func;
    ArgTypes args;
    Imp imp;
    bool pure;
    
    FuncImp(Func &fn,
	    const ArgTypes &args,
	    Imp imp,
	    bool pure=true);
    void operator ()(Scope &scp, const Args &args);
    void operator ()(Scope &scp);
  };

  struct Func {
    const Sym name;
    std::deque<FuncImp> imps;
    Func(const Sym &n);
  };

  Type *get_type(const FuncImp &imp, const ArgType &arg_type, const Args &args);
  FuncImp &add_imp(Func &fn,
		   const ArgTypes &args,
		   FuncImp::Imp imp);
  opt<Args> match(const FuncImp &imp, Thread &thd, bool conv_args);
  opt<std::pair<FuncImp *, Args>> match(Func &fn,
					Thread &thd,
					bool conv_args=false);
}

#endif
