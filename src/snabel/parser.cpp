#include <iostream>

#include "snabel/error.hpp"
#include "snabel/exec.hpp"
#include "snabel/parser.hpp"
#include "snabel/type.hpp"

namespace snabel {
  Pos::Pos(int64_t row, int64_t col):
    row(row), col(col)
  { }

  Tok::Tok(const str &text, Pos pos):
    text(text), pos(pos)
  { }

  static void parse_quote(const str &in,
			  size_t &lnr,
			  size_t start,
			  size_t &i,
			  char end,
			  TokSeq &out) {
    char pc(0);
    
    for (; i < in.size(); i++) {
      auto &c(in[i]);

      if (c == end && pc != '\\') {
	out.emplace_back(in.substr(start, i-start+1), Pos(lnr, start));
	break;
      } else if (c == '\n') {
	lnr++;
      }

      pc = c;
    }
  }

  static void parse_types(const str &in, size_t &lnr, size_t &i) {
    char pc(0);
    auto depth(1);
    i++;
    
    for (; i < in.size(); i++) {
      auto &c(in[i]);

      if (c == '>') {
	depth--;
	if (!depth) {
	  i++;
	  break;
	}
      } else if (c == '<') {
	depth++;
      } else if (c == '\n') {
	lnr++;
      }

      pc = c;
    }
  }

  void parse_expr(const str &in, size_t lnr, TokSeq &out) {
    static const std::set<char> split {
      '{', '}', '(', ')', '[', ']', '|', ';', '.' };

    size_t i(0), j(0);
    char pc(0);
    
    auto push_id = [&]() {
      if (j > i) {
	out.emplace_back(in.substr(i, j-i), Pos(lnr, i));
	i = j;
      }
    };

    for (; j < in.size(); j++) {
      auto &c(in[j]);

      if (c == ' ' || c == '\t') {
	push_id();
	i++;
      } else if (c == '\n') {
	push_id();
	i++;
	lnr++;
      } else if (c == '/' && pc == '/') {
	j--;
	push_id();
	j = in.find('\n', i);

	if (j == str::npos) {
	  j = in.size();
	}

	out.emplace_back(in.substr(i, j-i), Pos(lnr, i));
	i = j+1;	 
      } else if (c == '*' && pc == '/') {
	j--;
	push_id();
	j = in.find("*/", i);

	if (j == str::npos) {
	  ERROR(Snabel, fmt("Open comment at row %0, col %1", lnr, i));
	  break;
	}

	out.emplace_back(in.substr(i, j-i+2), Pos(lnr, i));
	j++;
	i = j+1;
      } else if (c == '\'') {
	auto k(j);

	if (pc == 'u') {
	  k--;
	  j--;
	}
	
	push_id();
	j += (pc == 'u') ? 2 : 1;
	parse_quote(in, lnr, k, j, '\'', out);
	i = j+1;
      } else if (c == '<') {
	parse_types(in, lnr, j);
	push_id();
	i++;
      } else if (split.find(c) != split.end()) {
	push_id();
	out.emplace_back(in.substr(i, 1), Pos(lnr, i));
	i++;
      } else if (c == '!' && pc == '#') {
	j = in.find('\n', j);
	if (j == str::npos) { j = in.size(); }
	i = j+1;
      }

      pc = c;
    }

    push_id();
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

  std::pair<Type *, size_t> parse_type(Exec &exe, const str &in, size_t i) {
    while (in[i] == ' ') { i++; }

    if (!in.empty() && in.at(i) == '>') {
      return std::make_pair(nullptr, i+1);
    }
    
    auto j(in.find('<', i));
    bool args_fnd(true);
    
    if (j == str::npos) {
      auto k(in.find(' ', i));

      if (k == str::npos) {
	j = in.size();
	if (in.back() == '>') { j--; }
      } else {
	j = k;
      }

      args_fnd = false;
    }

    str n(in.substr(i, j-i));
    auto fnd(find_type(exe, get_sym(exe, n)));

    if (!fnd) {
      ERROR(Snabel, fmt("Type not found: %0", n));
      return std::make_pair(nullptr, in.size());
    }

    Types args;
    i = j+1;
    
    if (args_fnd) {  
      while (i < in.size()) {
	auto res(parse_type(exe, in, i));
	i = res.second;
	if (!res.first) { break; }
	args.push_back(res.first);
      }
    }

    return std::make_pair(args.empty() ? fnd : &get_type(exe, *fnd, args), i);
  }
}

namespace snackis {
  template <>
  str fmt_arg(const snabel::Pos &arg) {
    return fmt("%0:%1", arg.row, arg.col);
  }
  
  template <>
  str fmt_arg(const snabel::Tok &arg) {
    return fmt("%0/%1", arg.text, arg.pos);
  }
}
