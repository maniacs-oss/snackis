#ifndef SNABEL_PARSER_HPP
#define SNABEL_PARSER_HPP

#include <deque>
#include "snackis/core/str.hpp"

namespace snabel {
  using namespace snackis;
  
  struct Pos {
    size_t row, col;
    Pos(size_t row, size_t col);
  };

  struct Tok {
    str text;
    size_t start;
    
    Tok(const str &text, size_t start=-1);
  };

  using StrSeq = std::deque<str>;
  using TokSeq = std::deque<Tok>;

  StrSeq parse_lines(const str &in);
  size_t parse_parens(const str &in);
  TokSeq parse_expr(const str &in);
}

#endif
