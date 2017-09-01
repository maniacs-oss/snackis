#ifndef SNABEL_NET_HPP
#define SNABEL_NET_HPP

namespace snabel {
  struct Exec;

  struct AcceptIter: Iter {
    FileRef in;
    
    AcceptIter(Exec &exe, const FileRef &in);
    opt<Box> next(Scope &scp) override;
  };

  void init_net(Exec &exe);
}

#endif
