#ifndef SNABEL_PARSER_HPP
#define SNABEL_PARSER_HPP

#include <deque>
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Pos {
    int64_t row, col;
    Pos(int64_t row, int64_t col);
  };

  struct Tok {
    str text;
    Pos pos;
    
    Tok(const str &text, Pos pos=Pos(-1,-1));
  };

  using StrSeq = std::deque<str>;
  using TokSeq = std::deque<Tok>;

  StrSeq parse_lines(const str &in);
  size_t parse_parens(const str &in);
  void parse_expr(const str &in, size_t lnr, TokSeq &out);
  TokSeq parse_expr(const str &in, size_t lnr=0);
  TokSeq::iterator find_end(TokSeq::iterator i,
			    const TokSeq::const_iterator &end);
}

#endif
