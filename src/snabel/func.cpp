#include <iostream>

#include "snabel/box.hpp"
#include "snabel/coro.hpp"
#include "snabel/scope.hpp"
#include "snabel/error.hpp"
#include "snabel/func.hpp"
#include "snabel/type.hpp"

namespace snabel {
  ArgType arg_type_0(0);
  ArgType arg_type_0_0(0, 0);

  FuncImp::FuncImp(Func &fn,
		   const ArgTypes &args,
		   const ArgTypes &results,
		   Imp imp,
		   bool pure):
    func(fn), args(args), results(results), imp(imp), pure(pure)
  { }
  
  void FuncImp::operator ()(Coro &cor, const Args &args) {
    Scope &tmp(begin_scope(cor, false));
    imp(tmp, *this, args);
    end_scope(cor, results.size());
  }

  void FuncImp::operator ()(Coro &cor) {
    auto args(pop_args(*this, cor));
    (*this)(cor, args);
  }

  Func::Func(const str &nam):
    name(nam)
  { }

  ArgType::ArgType(Type &type):
    type(&type)
  { }

  ArgType::ArgType(size_t arg_idx, opt<size_t> type_arg_idx):
    type(nullptr), arg_idx(arg_idx), type_arg_idx(type_arg_idx)
  { }

  Type *get_type(const FuncImp &imp, const ArgType &arg_type, const Args &args) {
    if (arg_type.type) { return arg_type.type; }
    auto t(args.at(args.size() - imp.args.size() + *arg_type.arg_idx).type);
    return arg_type.type_arg_idx ? t->args.at(*arg_type.type_arg_idx) : t;
  }

  Args pop_args(const FuncImp &imp, Coro &cor) {
    auto i = imp.args.rbegin();
    auto &s(curr_stack(cor));
    Args out;
    
    while (i != imp.args.rend() && !s.empty()) {
      out.push_front(s.back());
      s.pop_back();
      i++;
    }

    return out;
  }

  FuncImp &add_imp(Func &fn,
		   const ArgTypes &args,
		   const ArgTypes &results,
		   FuncImp::Imp imp) {
    return fn.imps.emplace_front(fn, args, results, imp);
  }

  bool match(const FuncImp &imp, const Coro &cor) {
    if (imp.args.empty()) { return true; }
    auto &s(curr_stack(cor));
    if (s.size() < imp.args.size()) { return false; }
    auto i(s.rbegin());
    auto j(imp.args.rbegin());
    
    while (i != s.rend() && j != imp.args.rend()) {
      auto t(get_type(imp, *j, s));
      if (!t || !isa(*i, *t)) { return false; }
      
      i++;
      j++;
    }

    return true;
  }

  FuncImp *match(Func &fn, const Coro &cor) {
    for (auto &imp: fn.imps) {
      if (match(imp, cor)) { return &imp; }
    }

    return nullptr;
  }
}
