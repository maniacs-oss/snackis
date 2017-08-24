#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/proc.hpp"
#include "snabel/label.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Proc::Proc(Label &tgt):
    id(uid(tgt.exec)), target(tgt), coro(nullptr)
  { }

  bool call(Proc &prc, Scope &scp, bool now) {
    if (!prc.coro) {
      prc.coro = &add_coro(scp, prc.target);
      prc.coro->proc = &prc;
    }

    call(scp, prc.target, now);
    return prc.coro ? true : false;
  }
}
