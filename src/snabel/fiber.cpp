#include "snabel/fiber.hpp"

namespace snabel {
  Fiber::Fiber(Exec &exe, Sym id):
    Coro(exe), id(id)
  { }
}
