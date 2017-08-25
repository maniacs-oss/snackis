#include "snabel/random.hpp"
#include "snabel/thread.hpp"

namespace snabel {
  int64_t pop(Random &rnd, Thread &thd){
    return rnd(thd.random);
  }
}
