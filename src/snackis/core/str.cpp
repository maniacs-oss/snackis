#include <sodium.h>

#include "snackis/core/error.hpp"
#include "snackis/core/str.hpp"

namespace snackis {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> uconv;
  const str whitespace(" \t\r\n");
  
  str trim(const str& in) {
    const size_t i = in.find_first_not_of(whitespace);
    if (i == str::npos) { return ""; }
    const size_t j = in.find_last_not_of(whitespace);
    return in.substr(i, (j-i+1));
  }

  str fill(const str &in, char ch, size_t len) {
    str out(in);
    const auto rem(out.size() % len);
    if (rem || in.empty()) { out.append(len - rem, ch); }
    return out;
  }

  void replace(str &in, const str &from, const str &to) {
    size_t i(0);
    
    while ((i = in.find(from, i)) != std::string::npos) {
      in.replace(i, from.length(), to);
      i += to.length();
    }
  }

  size_t find_ci(const str &stack, const str& needle) {
    auto found(std::search(stack.begin(), stack.end(),
		       needle.begin(), needle.end(),
		       [](char x, char y) {
			 return std::tolower(x) == std::tolower(y);
			   }));
    return (found == stack.end()) ? str::npos : found - stack.begin();
  }

  std::set<str> word_set(const str &in) {
    InStream in_words(in);
    std::set<str> out;
    str w;
    while (in_words >> w) { out.insert(w); }
    return out;
  }
  
  int64_t to_int64(const str &in) {
    return strtoll(in.c_str(), nullptr, 10);
  }

  str bin_hex(const unsigned char *in, size_t len) {
    const size_t out_len = len*2+1;
    Data out(out_len, 0);
    
    if (!sodium_bin2hex(reinterpret_cast<char *>(&out[0]), out_len, in, len)) {
      ERROR(Core, "Hex-encoding failed");
    }

    return str(reinterpret_cast<const char *>(&out[0]));
  }
  
  Data hex_bin(const str &in) {
    Data out(in.size()/2, 0);
    size_t len;
    
    if (sodium_hex2bin(&out[0], out.size(),
		       in.c_str(), in.size(),
		       nullptr,
		       &len,
		       nullptr)) {
      ERROR(Core, "Hex-decoding failed");
    }

    out.resize(len);
    return out;
  }

  size_t prefix_len(str x, str y) {
    if( x.size() > y.size() ) std::swap(x,y) ;
    return std::mismatch(x.begin(), x.end(), y.begin()).first - x.begin();
  }
}
