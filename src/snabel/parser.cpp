#include <iostream>

#include "snabel/error.hpp"
#include "snabel/parser.hpp"

namespace snabel {
  Pos::Pos(int64_t row, int64_t col):
    row(row), col(col)
  { }

  Tok::Tok(const str &text, Pos pos):
    text(text), pos(pos)
  { }

  StrSeq parse_lines(const str &in) {
    size_t i(0), j(0);
    StrSeq out;
    
    while ((j=in.find('\n', j)) != str::npos) {
      if (j > 0 && in[j-1] == '\\') {
	j += 2;
      } else if (j > i) {
	out.push_back(trim(in.substr(i, j-i)));
	j++;
	i = j;
      } else {
	j++;
	i = j;
      }
    }

    if (i < in.size()) { out.push_back(trim(in.substr(i))); }
    return out;
  }

  size_t parse_pair(const str &in, char fst, char lst) {
    bool quoted(false);
    int depth(1);
    
    for (size_t i(1); i < in.size(); i++) {
      auto &c(in[i]);
      
      if (c == '\'') {
	if (i == 0 || in[i-1] != '\\') { quoted = !quoted; }
      } else if (c == fst) {
	depth++;
      } else if (c == lst) {
	depth--;
	if (!depth) { return i; }
      }
    }

    return str::npos;
  }
  
  void parse_expr(const str &in, size_t lnr, TokSeq &out) {
    static const std::set<char> split {
      '{', '}', '(', ')', '[', ']', '<', '>', '|', ';', '!', '.' };
    
    size_t i(0);
    bool quoted(false);
    
    for (size_t j(0); j < in.size(); j++) {
      char c(in[j]);

      while (quoted && c != '\'') {
	j++;
	c = in[j];
      }

      if (c == '\'' && (j == 0 || in[j-1] != '\\')) { quoted = !quoted; }
      const size_t cp(j);

      if ((split.find(c) != split.end() ||
	   c == '\'' || c == '\n' || c == ' ') &&
	  !quoted && j > i) {
	if (split.find(in[i]) != split.end()) { i++; }
	const str s(trim(in.substr(i, (c == '\'') ? j-i+1 : j-i)));
	if (!s.empty()) { out.emplace_back(s, Pos(lnr, i)); }
	i = (c == '\'') ? j+1 : j;
      }

      if (split.find(c) != split.end()) {
	out.emplace_back(in.substr(cp, 1), Pos(lnr, cp));
      }

      if ((i < j || in.size() == 1) && j == in.size()-1) {
	str s(trim(in.substr(i)));
	if (!s.empty()) { out.emplace_back(s, Pos(lnr, i)); }
      }
    }
  }

  TokSeq parse_expr(const str &in, size_t lnr) {
    TokSeq out;
    parse_expr(in, lnr, out);
    return out;
  }
  
  TokSeq::iterator find_end(TokSeq::iterator i,
			    const TokSeq::const_iterator &end) {
    int depth(1);
    
    for (; i != end; i++) {
      if (i->text.back() == ':') { depth++; }
      
      if (i->text.front() == ';') {
	depth--;
	if (!depth) { break; }
      }
    }
    
    return i;
  }
}
