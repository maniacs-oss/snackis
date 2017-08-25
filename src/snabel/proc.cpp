#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/proc.hpp"
#include "snabel/label.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Proc::Proc(const CoroRef &cor):
    id(uid(cor->target.exec)), coro(cor)
  { }

  bool call(const ProcRef &prc, Scope &scp, bool now) {
    prc->coro->proc = prc;
    return call(prc->coro, scp, now);
  }
}
