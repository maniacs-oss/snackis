#ifndef SNABEL_THREAD_HPP
#define SNABEL_THREAD_HPP

#include <map>
#include <thread>
#include "snabel/fiber.hpp"

namespace snabel {  
  struct Thread {
    using Id = Sym;
  
    std::thread imp;
    std::map<Fiber::Id, Fiber> fibers;

    Exec &exec;
    const Id id;
    OpSeq ops;
    int64_t pc;    
    Fiber &main, *curr_fiber;
    Scope &main_scope;
    
    Thread(Exec &exe, Id id);
  };

  void start(Thread &thd);
  void join(Thread &thd, Scope &scp);
  bool run(Thread &thd, bool scope=true); 
}
 
#endif
