#include <iostream>
#include "snabel/compiler.hpp"
#include "snabel/error.hpp"
#include "snabel/exec.hpp"

namespace snabel {  
  static void compile_tok(Exec &exe,
			  TokSeq &in,
			  OpSeq &out) {
    Tok tok(in.at(0));
    in.pop_front();
    
    if (tok.text.at(0) == '"') {
      // Skip comments
    } else if (tok.text.at(0) == '#') {
      out.emplace_back(Getenv(tok.text));
    } else if (tok.text == "&_") {
      out.emplace_back(Push(Box(exe.drop_type, n_a)));
    }
    else if (tok.text.substr(0, 6) == "return" &&
	       tok.text.size() == 7 &&
	       isdigit(tok.text.at(6))) {
      auto i(tok.text.at(6) - '0');
      out.emplace_back(Return(i+1));
    } else if (tok.text == "&return") {
      out.emplace_back(Push(Box(exe.label_type, exe.return_target[0])));
    } else if (tok.text.substr(0, 7) == "&return" &&
	       tok.text.size() == 8 &&
	       isdigit(tok.text.at(7))) {
      auto i(tok.text.at(7) - '0');
      out.emplace_back(Push(Box(exe.label_type, exe.return_target[i])));
    } else if (tok.text.substr(0, 6) == "recall" &&
	       tok.text.size() == 7 &&
	       isdigit(tok.text.at(6))) {
      auto i(tok.text.at(6) - '0');
      out.emplace_back(Recall(i+1));
    } else if (tok.text == "&recall") {
      out.emplace_back(Push(Box(exe.label_type, exe.recall_target[0])));
    } else if (tok.text.substr(0, 7) == "&recall" &&
	       tok.text.size() == 8 &&
	       isdigit(tok.text.at(7))) {
      auto i(tok.text.at(7) - '0');
      out.emplace_back(Push(Box(exe.label_type, exe.recall_target[i])));
    } else if (tok.text.substr(0, 5) == "yield" &&
	       tok.text.size() == 6 &&
	       isdigit(tok.text.at(5))) {
      auto i(tok.text.at(5) - '0');
      out.emplace_back(Yield(i+1));
    } else if (tok.text == "&yield") {
      out.emplace_back(Push(Box(exe.label_type, exe.yield_target[0])));
    } else if (tok.text.substr(0, 6) == "&yield" &&
	       tok.text.size() == 7 &&
	       isdigit(tok.text.at(6))) {
      auto i(tok.text.at(6) - '0');
      out.emplace_back(Push(Box(exe.label_type, exe.yield_target[i])));
    } else if (tok.text.substr(0, 5) == "break" &&
	       tok.text.size() == 6 &&
	       isdigit(tok.text.at(5))) {
      auto i(tok.text.at(5) - '0');
      out.emplace_back(Break(i+1));
    } else if (tok.text == "&break") {
      out.emplace_back(Push(Box(exe.label_type, exe.break_target[0])));
    } else if (tok.text.substr(0, 6) == "&break" &&
	       tok.text.size() == 7 &&
	       isdigit(tok.text.at(6))) {
      auto i(tok.text.at(6) - '0');
      out.emplace_back(Push(Box(exe.label_type, exe.break_target[i])));
    } else if (tok.text.at(0) == '&') {
      out.emplace_back(Getenv(tok.text.substr(1)));
    } else if (tok.text.at(0) == '@') {
      out.emplace_back(Getenv(tok.text));
    } else if (tok.text.at(0) == '$' &&
	       tok.text.size() == 2 &&
	       isdigit(tok.text.at(1))) {
      auto i(tok.text.at(1) - '0');
      if (i) {
	out.emplace_back(Swap(i));
      } else {
	out.emplace_back(Dup());
      }
    } else if (tok.text.at(0) == '\'') {
      auto s(tok.text.substr(1, tok.text.size()-2));
      replace(s, "\\n", "\n");
      replace(s, "\\r", "\r");
      replace(s, "\\t", "\t");
      out.emplace_back(Push(Box(exe.str_type, s)));
    } else if (tok.text.at(0) == 'u' && tok.text.at(1) == '\'') {
      auto v(tok.text.substr(2, tok.text.size()-3));
      out.emplace_back(Push(Box(exe.ustr_type, uconv.from_bytes(v))));
    } else if (tok.text.at(0) == '\\') {
      if (tok.text.size() < 2) {
	ERROR(Snabel, fmt("Invalid char literal on row %0, col %1: %2",
			  tok.pos.row, tok.pos.col, tok.text));
      }

      char c(0);

      if (tok.text == "\\space") {
	c = ' ';
      } else if (tok.text == "\\n") {
	c = '\n';
      } else if (tok.text == "\\t") {
	c = '\t';
      } else {
	c = tok.text[1];
      }
      
      out.emplace_back(Push(Box(exe.char_type, c)));
    } else if (tok.text.at(0) == 'u' && tok.text.at(1) == '\\') {
      if (tok.text.size() < 3) {
	ERROR(Snabel, fmt("Invalid uchar literal on row %0, col %1: %2",
			  tok.pos.row, tok.pos.col, tok.text));
      }

      uchar c(0);

      if (tok.text == "u\\space") {
	c = u' ';
      } else if (tok.text == "u\\n") {
	c = u'\n';
      } else if (tok.text == "u\\t") {
	c = u'\t';
      } else {
	c = uconv.from_bytes(str(1, tok.text[1])).at(0);
      }
      
      out.emplace_back(Push(Box(exe.uchar_type, c)));
    } else if (isupper(tok.text[0])) {
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
	fnd->second(tok.pos, in, out);
      }
    }
  }

  void compile(Exec &exe, TokSeq in, OpSeq &out) {
    while (!in.empty()) { compile_tok(exe, in, out); }
  }
}
