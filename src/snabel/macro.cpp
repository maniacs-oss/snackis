#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/lambda.hpp"
#include "snabel/macro.hpp"

namespace snabel {
  Macro::Macro(Imp imp):
    imp(imp)
  { }

  static void target_imp(Exec &exe,
			 const LambdaRef &lmb,
			 Pos pos,
			 TokSeq &in,
			 OpSeq &out) {
    auto &s(curr_stack(exe.main));
    s.clear();
    call(lmb, exe.main_scope, true);
    
    for (auto v: s) {
      if (v.type == &exe.quote_type) {
	TokSeq toks;
	parse_expr(*get<StrRef>(v), 0, toks);
	compile(exe, toks, out);
      } else {
	out.emplace_back(Push(v));
      }
    }

    s.clear();
  }
  
  Macro::Macro(Exec &exe, const LambdaRef &lmb):
    imp([&exe, lmb](auto pos, auto &in, auto &out) {
	target_imp(exe, lmb, pos, in, out);
      })
  { }

  void Macro::operator ()(Pos pos, TokSeq &in, OpSeq &out) const {
    imp(pos, in, out);
  }
}
