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

  void upcase(str &in);
  void downcase(str &in);
  
  template <typename IterT, typename T>
  str join(IterT beg, IterT end, T sep) {
    OutStream out;

    for (auto i(beg); i != end; i++) {
      if (i != beg) { out << sep; }
      out << *i;
    }
    
    return out.str();
  }

  template <typename T>
  bool suffix(const T &in, const T &sfx) {    
    if (sfx.size() > in.size()) return false;
    return std::equal(sfx.rbegin(), sfx.rend(), in.rbegin());
  }

  template <typename T>
  void replace(T &in, const T &from, const T &to) {
    size_t i(0);
    
    while ((i = in.find(from, i)) != T::npos) {
      in.replace(i, from.length(), to);
      i += to.length();
    }
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
