#include <iostream>

#include "snabel/exec.hpp"
#include "snabel/fiber.hpp"
#include "snabel/label.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Fiber::Fiber(Label &tgt):
    id(gensym(tgt.exec)), target(tgt), coro(nullptr)
  { }

  bool call(Fiber &fib, Scope &scp) {
    if (!fib.coro) {
      fib.coro = &add_coro(scp, fib.target);
      fib.coro->fiber = &fib;
      fib.result.reset();
    }

    call(scp, fib.target);
    return !fib.coro;
  }
}
