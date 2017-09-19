#ifndef SNABEL_NET_HPP
#define SNABEL_NET_HPP

#include <netinet/in.h>

namespace snabel {
  struct Exec;
  struct Thread;
  
  struct AcceptIter: Iter {
    FileRef in;
    
    AcceptIter(Thread &thd, const FileRef &in);
    opt<Box> next(Scope &scp) override;
  };

  struct ConnectIter: Iter {
    FileRef f;
    sockaddr_in addr;
    
    ConnectIter(Thread &thd, const FileRef &f, sockaddr_in addr);
    opt<Box> next(Scope &scp) override;
  };

  void init_net(Exec &exe);
}

#endif
