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
    
    if (tok.text.front() == '@') {
      out.emplace_back(Target(tok.text.substr(1)));
    } else if (tok.text.front() == '&') {
      out.emplace_back(Pointer(tok.text.substr(1)));
    } else if (tok.text.front() == '"') {
      out.emplace_back(Push(Box(exe.str_type,
				tok.text.substr(1, tok.text.size()-2))));
    } else if (isdigit(tok.text[0]) || 
	(tok.text.size() > 1 && tok.text[0] == '-' && isdigit(tok.text[1]))) {
      out.emplace_back(Push(Box(exe.i64_type, to_int64(tok.text))));
    } else {
      auto fnd(exe.macros.find(tok.text));
      
      if (fnd == exe.macros.end()) {
	out.emplace_back(Get(tok.text));
      } else {
	fnd->second(Pos(lnr, tok.i), in, out);
      }
    }
  }

  void compile(Exec &exe, size_t lnr, TokSeq in, OpSeq &out) {
    while (!in.empty()) { compile_tok(exe, lnr, in, out); }
  }
}
