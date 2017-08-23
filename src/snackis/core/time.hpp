#ifndef SNACKIS_TIME_HPP
#define SNACKIS_TIME_HPP

#include <chrono>
#include "snackis/core/fmt.hpp"
#include "snackis/core/str.hpp"

namespace snackis {
  using Clock = std::chrono::system_clock;
  using PClock = std::chrono::high_resolution_clock;
  using Time = std::chrono::time_point<Clock>;
  using PTime = std::chrono::time_point<PClock>;

  extern const Time null_time, max_time;
  
  PTime pnow();
  Time now();
  str fmt(const Time &time, const str &spec);
  opt<Time> parse_time(const str &spec, const str &in, opt<Time> empty=nullopt);

  template <>
  str fmt_arg(const Time &arg);

  template <typename...Args>
  auto usecs(std::chrono::duration<Args...> d) {
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
  }
};

#endif
