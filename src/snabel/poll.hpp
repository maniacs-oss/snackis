#ifndef SNABEL_POLL_HPP
#define SNABEL_POLL_HPP

#include <vector>
#include <poll.h>

namespace snabel {
  using FDSet = std::vector<pollfd>;
}

#endif
