#include "snabel/fiber.hpp"

namespace snabel {
  Fiber::Fiber(Thread &thd, Id id, Label &tgt):
    thread(thd), id(id), target(tgt)
  { }
}
