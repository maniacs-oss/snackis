#ifndef SNABEL_FUNC_HPP
#define SNABEL_FUNC_HPP

#include <list>
#include <vector>

#include "snabel/error.hpp"
#include "snabel/refs.hpp"
#include "snabel/sym.hpp"
#include "snabel/type.hpp"
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
  
  using Args = std::vector<Box>;

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
  
  using ArgTypes = std::vector<ArgType>;

  struct FuncImp {
    using Imp = func<void (Scope &, const Args &)>;
    
    Func &func;
    const int sec;
    ArgTypes args;
    Imp imp;
    LambdaRef lambda;
    
    FuncImp(Func &fn,
	    int sec,
	    const ArgTypes &args,
	    Imp imp);
    
    FuncImp(Func &fn,
	    int sec,
	    const ArgTypes &args,
	    const LambdaRef &lmb);

    bool operator ()(Scope &scp, const Args &args);
  };

  struct Func {
    using Imps = std::list<FuncImp>;
    using ImpHandle = Imps::iterator;
    
    static const int Pure;
    static const int Const;
    static const int Safe;
    static const int Unsafe;

    const Sym name;
    Imps imps;
    Func(const Sym &n);
  };

  Type *get_type(const FuncImp &imp, const ArgType &arg_type, const Args &args);
  opt<Args> match(const FuncImp &imp, Types types, Scope &scp, bool conv_args);

  opt<std::pair<FuncImp *, Args>> match(Func &fn,
					const Types &types,
					Scope &scp,
					bool conv_args=false);

  template <typename T>
  Func::ImpHandle add_imp(Func &fn,
			  int sec,
			  const ArgTypes &args,
			  T imp) {
    fn.imps.emplace_front(fn, sec, args, imp);
    return fn.imps.begin();
  }
}

#endif
