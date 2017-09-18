#include <iostream>

#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/func.hpp"
#include "snabel/scope.hpp"
#include "snabel/type.hpp"

namespace snabel {
  const int Func::Pure(0), Func::Const(1), Func::Safe(2), Func::Unsafe(3);

  FuncImp::FuncImp(Func &fn,
		   int sec,
		   const ArgTypes &args,
		   Imp imp):
    func(fn), sec(sec), args(args), imp(imp)
  { }

  FuncImp::FuncImp(Func &fn,
		   int sec,
		   const ArgTypes &args,
		   const LambdaRef &lmb):
    func(fn), sec(sec), args(args), lambda(lmb)
  { }

  bool FuncImp::operator ()(Scope &scp, const Args &args) {
    if (lambda) {
      return call(lambda, scp, true);
    }

    auto &s(curr_stack(scp.thread));
    s.erase(std::next(s.begin(), s.size()-args.size()), s.end());
    imp(scp, args);
    return true;
  }

  Func::Func(const Sym &n):
    name(n)
  { }

  ArgType::ArgType(Type &type):
    type(&type)
  { }

  ArgType::ArgType(size_t arg_idx):
    type(nullptr), arg_idx(arg_idx)
  { }

  ArgType::ArgType(size_t arg_idx, size_t type_arg_idx):
    type(nullptr), arg_idx(arg_idx), type_arg_idx(type_arg_idx)
  { }

  ArgType::ArgType(Fn fn):
    type(nullptr), fn(fn)
  { }

  Type *get_type(const FuncImp &imp, const ArgType &arg_type, const Args &args) {
    Type *t(nullptr);
    
    if (arg_type.fn) {
      t = (*arg_type.fn)(args);
    } else if (arg_type.type) {
      t = arg_type.type;
    } else {
      t = args.at(*arg_type.arg_idx).type;
      
      if (arg_type.type_arg_idx) {
	if (t->args.size() > *arg_type.type_arg_idx) {
	  t = t->args.at(*arg_type.type_arg_idx);
	} else {
	  t = nullptr;
	}
      }
    }
    
    return t;
  }

  FuncImp &add_imp(Func &fn,
		   int sec,
		   const ArgTypes &args,
		   FuncImp::Imp imp) {
    return fn.imps.emplace_front(fn, sec, args, imp);
  }

  FuncImp &add_imp(Func &fn,
		   int sec,
		   const ArgTypes &args,
		   const LambdaRef &lmb) {
    return fn.imps.emplace_front(fn, sec, args, lmb);
  }

  opt<Args> match(const FuncImp &imp,
		  Types types,
		  Scope &scp,
		  bool conv_args) {
    auto &exe(scp.exec);
    auto &thd(scp.thread);
    auto &s(curr_stack(thd));
    if (s.size() < imp.args.size()) { return nullopt; }
    Args as(std::next(s.begin(), s.size()-imp.args.size()), s.end());
    if (imp.args.empty()) { return as; }
    auto i(as.rbegin());
    auto j(imp.args.rbegin());
    
    for (int i(types.size()); i < imp.args.size(); i++) {
      types.push_back(&exe.any_type);
    }
    
    auto ti(types.rbegin());
    
    while (i != as.rend() && j != imp.args.rend()) {
      auto &a(*i);
      if (a.safe_level != scp.safe_level) { ERROR(UnsafeStack); }
      
      auto t(get_type(imp, *j, as));

      if (ti != types.rend()) {
	if (*ti != &exe.any_type && (!j->type || !isa(thd, **ti, *j->type))) {
	  return nullopt;
	}

	ti++;
      }

      if (!t || (!isa(thd, a, *t) && (!conv_args || !conv(scp, a, *t)))) {
	return nullopt;
      }
      
      i++;
      j++;
    }

    return as;
  }
  
  opt<std::pair<FuncImp *, Args>> match(Func &fn,
					const Types &types,
					Scope &scp,
					bool conv_args) {
    for (auto &imp: fn.imps) {
      auto args(match(imp, types, scp, conv_args));
      if (args) { return std::make_pair(&imp, *args); }
    }

    return conv_args ? nullopt : match(fn, types, scp, true);
  }
}
