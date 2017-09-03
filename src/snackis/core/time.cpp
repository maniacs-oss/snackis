#include <ctime>
#include <iomanip>
#include "snackis/core/stream.hpp"
#include "snackis/core/time.hpp"

namespace snackis {
  const Time null_time, max_time(Time::max());

  Time now() {
    Time tim(Clock::now());
    return std::chrono::time_point_cast<std::chrono::milliseconds>(tim);
  }

  PTime pnow() { return PClock::now(); }

  std::chrono::microseconds usecs(size_t n) {
    return std::chrono::microseconds(n);
  }

  str fmt(const Time &tim, const str &spec) {
    if (tim == null_time || tim == max_time) { return ""; }
    Stream buf;
    auto t(Clock::to_time_t(tim));
    tm tm;
    memset(&tm, 0, sizeof(tm));
    localtime_r(&t, &tm);
    buf << std::put_time(localtime_r(&t, &tm), spec.c_str());
    return buf.str();
  }

  opt<Time> parse_time(const str &spec, const str &in, opt<Time> empty) {
    if (in.empty()) { return empty; }
    InStream buf(in);
    tm tm;
    memset(&tm, 0, sizeof(tm));
    buf >> std::get_time(&tm, spec.c_str());
    if (buf.fail()) { return nullopt; }
    return Clock::from_time_t(mktime(&tm));
  }

  template <>
  str fmt_arg(const Time &arg) {
    return fmt(arg, "%Y-%m-%d %H:%M");
  }
}
