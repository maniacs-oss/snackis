#include <iostream>

#include "snabel/box.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/func.hpp"
#include "snabel/scope.hpp"
#include "snabel/type.hpp"

namespace snabel {
  FuncImp::FuncImp(Func &fn,
		   const ArgTypes &args,
		   const ArgTypes &results,
		   Imp imp,
		   bool pure):
    func(fn), args(args), results(results), imp(imp), pure(pure)
  { }
  
  void FuncImp::operator ()(Scope &scp, const Args &args) {
    auto &s(curr_stack(scp.thread));
    s.erase(std::next(s.begin(), s.size()-args.size()), s.end());
    imp(scp, args);
  }

  Func::Func(const str &nam):
    name(nam)
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
		   const ArgTypes &args,
		   const ArgTypes &results,
		   FuncImp::Imp imp) {
    return fn.imps.emplace_front(fn, args, results, imp);
  }

  opt<Args> match(const FuncImp &imp, const Thread &thd, bool conv_args) {
    auto &exe(thd.exec);
    auto &s(curr_stack(thd));
    if (s.size() < imp.args.size()) { return nullopt; }
    Args in(std::next(s.begin(), s.size()-imp.args.size()), s.end());
    if (imp.args.empty()) { return in; }
    auto i(s.rbegin());
    auto j(imp.args.rbegin());
    Args out;
    
    while (i != s.rend() && j != imp.args.rend()) {
      auto a(*i);
      auto t(get_type(imp, *j, in));
  
      if (!t || (!isa(a, *t) && (!conv_args || !conv(exe, a, *t)))) {
	return nullopt;
      }
      
      out.push_front(a);      
      i++;
      j++;
    }

    return out;
  }
  
  opt<std::pair<FuncImp *, Args>> match(Func &fn, const Thread &thd, bool conv_args) {
    for (auto &imp: fn.imps) {
      auto args(match(imp, thd, conv_args));
      if (args) { return std::make_pair(&imp, *args); }
    }

    return conv_args ? nullopt : match(fn, thd, true);
  }
}
