#include <iostream>

#include "snabel/fiber.hpp"
#include "snabel/label.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Fiber::Fiber(Thread &thd, Id id, Label &tgt):
    thread(thd), id(id), target(tgt), coro(nullptr)
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
