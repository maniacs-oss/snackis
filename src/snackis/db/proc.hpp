#ifndef SNACKIS_DB_PROC_HPP
#define SNACKIS_DB_PROC_HPP

#include "snackis/core/path.hpp"
#include "snackis/db/loop.hpp"

namespace snackis {
namespace db {
  struct Proc: Loop {
    using Logger = func<void (const str &)>;

    const Path path;
    opt<Logger> logger;

    Proc(const Path &p, size_t max_buf);
    void on_msg(const Msg &msg) override;
  };

  template <typename...Args>
  void log(const Proc &p, const str &spec, const Args&...args) {
    if (p.logger) { (*p.logger)(fmt(spec, std::forward<const Args &>(args)...)); }
  }
}}

#endif
