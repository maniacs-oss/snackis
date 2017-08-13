#include "snabel/fiber.hpp"

namespace snabel {
  Fiber::Fiber(Thread &thd, Id id):
    Coro(thd), id(id)
  { }
}
