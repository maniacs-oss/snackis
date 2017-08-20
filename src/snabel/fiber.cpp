#include "snabel/fiber.hpp"
#include "snabel/label.hpp"
#include "snabel/scope.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  Fiber::Fiber(Thread &thd, Id id, Label &tgt):
    thread(thd), id(id), target(tgt)
  { }

  void init(Fiber &fib, Scope &scp) {
    call(fib, scp);
    fib.coro = find_coro(scp, fib.target);
    if (fib.coro) { fib.coro->fiber = &fib; }
  }
  
  void call(Fiber &fib, Scope &scp) {
    call(scp, fib.target);
  }
}
