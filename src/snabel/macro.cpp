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
      if (!in.front().text.empty()) {
	auto t(in.front().text);
	s.push_back(Box(scp,
			exe.sym_type,
			get_sym(exe, (t.front() == '#') ? t.substr(1) : t)));
      }
      
      in.pop_front();
    }

    if (nargs) { in.pop_front(); }
    call(lmb, curr_scope(curr_thread(exe)), true);
    OutStream buf;
    str sep("");
    
    for (auto v: s) {
      buf << sep;
      v.type->uneval(v, buf);
      sep = " ";
    }

    compile(exe, parse_expr(buf.str(), pos.row), out);
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
