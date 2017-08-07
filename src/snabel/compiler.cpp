#include <iostream>
#include "snabel/compiler.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"

namespace snabel {  
  static void compile_tok(Exec &exe,
			  size_t lnr,
			  TokSeq &in,
			  OpSeq &out) {
    Tok tok(in.front());
    in.pop_front();
    
    if (tok.text[0] == '{') {      
      out.push_back(Op::make_lambda());
      str e(tok.text.substr(1, tok.text.size()-2));
      compile(exe, lnr, parse_expr(e), out);
      out.push_back(Op::make_unlambda());
    } else if (tok.text.front() == '@') {
      out.push_back(Op::make_label(tok.text.substr(1)));
    } else if (tok.text.back() == '!') {
      out.push_back(Op::make_jump(tok.text.substr(0, tok.text.size()-1)));
    } else if (tok.text.front() == '"') {
      out.push_back(Op::make_push(Box(exe.str_type,
				      tok.text.substr(1, tok.text.size()-2))));
    } else if (isdigit(tok.text[0]) || 
	(tok.text.size() > 1 && tok.text[0] == '-' && isdigit(tok.text[1]))) {
      out.push_back(Op::make_push(Box(exe.i64_type, to_int64(tok.text))));
    } else {
      auto fnd(exe.macros.find(tok.text));
      
      if (fnd == exe.macros.end()) {
	out.push_back(Op::make_get(tok.text));
      } else {
	fnd->second(Pos(lnr, tok.i), in, out);
      }
    }
  }

  void compile(Exec &exe, size_t lnr, TokSeq in, OpSeq &out) {
    while (!in.empty()) { compile_tok(exe, lnr, in, out); }
  }
}
