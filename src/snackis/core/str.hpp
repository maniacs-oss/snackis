#ifndef SNACKIS_STR_HPP
#define SNACKIS_STR_HPP

#include <codecvt>
#include <set>
#include <string>

#include "snackis/core/char.hpp"
#include "snackis/core/data.hpp"
#include "snackis/core/opt.hpp"
#include "snackis/core/stream.hpp"

namespace snackis {
  using str = std::string;
  using ustr = std::u16string;
  extern const str whitespace;
  extern std::wstring_convert<std::codecvt_utf8_utf16<uchar>, uchar> uconv;

  str trim(const str& in);
  str fill(const str &in, char ch, size_t len);

  template <typename IterT, typename T>
  str join(IterT beg, IterT end, T sep) {
    OutStream out;

    for (auto i(beg); i != end; i++) {
      if (i != beg) { out << sep; }
      out << *i;
    }
    
    return out.str();
  }

  std::set<str> word_set(const str &in);

  size_t find_ci(const str &stack, const str& needle);
  int64_t to_int64(const str &in);

  template <typename T>
  str to_str(const T &in) {
    return std::to_string(in);
  }

  str bin_hex(const unsigned char *in, size_t len);
  Data hex_bin(const str &in);

  size_t prefix_len(str x, str y);
}

#endif
