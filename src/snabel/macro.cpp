#include <iostream>

#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/lambda.hpp"
#include "snabel/macro.hpp"

namespace snabel {
  Macro::Macro(Sym name, Imp imp):
    name(name), imp(imp), nargs(0)
  { }

  static void target_imp(Exec &exe,
			 int nargs,
			 const LambdaRef &lmb,
			 Pos pos,
			 TokSeq &in,
			 OpSeq &out) {
    auto &thd(curr_thread(exe));
    auto &scp(curr_scope(thd));
    auto &s(curr_stack(thd));
    s.clear();

    if (in.size() < nargs + (nargs ? 1 : 0)) {
      ERROR(Snabel, "Missing macro arguments");
      return;
    }
    
    for (int i(0); i < nargs; i++) {
      s.push_back(Box(scp, exe.tok_type, in.front()));
      in.pop_front();
    }

    if (nargs) { in.pop_front(); }
    call(lmb, curr_scope(curr_thread(exe)), true);
    
    for (auto v: s) {
      if (v.type == &exe.tok_type) {
	compile(exe, {get<Tok>(v)}, out);
      } else if (v.type == &exe.quote_type) {
	TokSeq toks;
	parse_expr(*get<StrRef>(v), 0, toks);
	compile(exe, toks, out);
      } else if (v.type == &exe.sym_type) {
	out.emplace_back(Deref(get<Sym>(v)));
      } else {
	out.emplace_back(Push(v));
      }
    }

    s.clear();
  }
  
  Macro::Macro(Exec &exe, Sym name, int nargs, const LambdaRef &lmb):
    name(name),
    imp([&exe, nargs, lmb](auto pos, auto &in, auto &out) {
	target_imp(exe, nargs, lmb, pos, in, out);
      }),
    nargs(nargs)
  { }

  void Macro::operator ()(Pos pos, TokSeq &in, OpSeq &out) const {
    imp(pos, in, out);
  }
}
