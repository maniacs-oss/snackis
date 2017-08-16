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
      out.emplace_back(Getenv(tok.text.substr(1)));
    } else if (tok.text.front() == '\'') {
      out.emplace_back(Push(Box(exe.str_type,
				tok.text.substr(1, tok.text.size()-2))));
    } else if (tok.text.front() == '\\') {
      if (tok.text.size() < 2) {
	ERROR(Snabel, fmt("Invalid char literal on row %0, col %1: %2",
			  lnr, tok.start, tok.text));
      }

      char c(0);

      if (tok.text == "\\space") {
	c = ' ';
      } else if (tok.text == "\\n") {
	c = '\n';
      } else if (tok.text == "\\t") {
	c = '\t';
      } else if (tok.text.size() > 2 && tok.text[1] == '\\') { 
	c = tok.text[2];
      } else {
	c = tok.text[1];
      }
      
      out.emplace_back(Push(Box(exe.char_type, c)));
    }
    else if (isupper(tok.text[0])) {
      auto fnd(find_type(exe, tok.text));

      if (fnd) {
	out.emplace_back(Push(Box(get_meta_type(exe, *fnd), fnd)));
      } else {
	ERROR(Snabel, fmt("Type not found: %0", tok.text));
      }
    } else if (isdigit(tok.text[0]) || 
	(tok.text.size() > 1 && tok.text[0] == '-' && isdigit(tok.text[1]))) {
      out.emplace_back(Push(Box(exe.i64_type, to_int64(tok.text))));
    } else {
      auto fnd(exe.macros.find(tok.text));
      
      if (fnd == exe.macros.end()) {
	out.emplace_back(Deref(tok.text));
      } else {
	fnd->second(Pos(lnr, tok.start), in, out);
      }
    }
  }

  void compile(Exec &exe, size_t lnr, TokSeq in, OpSeq &out) {
    while (!in.empty()) { compile_tok(exe, lnr, in, out); }
  }
}
