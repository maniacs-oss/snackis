#ifndef SNABEL_POLL_HPP
#define SNABEL_POLL_HPP

#include <list>

namespace snabel {
  struct File;
  
  using PollQueue = std::list<File *>;
  using PollHandle = PollQueue::iterator;

  const int POLL_SET_SIZE(10);
}

#endif
