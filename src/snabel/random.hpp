#ifndef SNABEL_RANDOM_HPP
#define SNABEL_RANDOM_HPP

#include <memory>
#include <random>

#include "snabel/iter.hpp"

namespace snabel {
  struct Thread;

  using Random = std::uniform_int_distribution<int64_t>;
  using RandomRef = std::shared_ptr<Random>;
  
  int64_t pop(Random &rnd, Thread &thd);
}

#endif
