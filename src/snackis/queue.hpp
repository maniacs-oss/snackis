#ifndef SNACKIS_QUEUE_HPP
#define SNACKIS_QUEUE_HPP

#include <set>

#include "snackis/rec.hpp"
#include "snackis/core/str.hpp"
#include "snackis/core/uid.hpp"

namespace snackis {
  struct Queue: Rec {
    UId id;
    UId owner_id;
    str name;
    str info;
    std::set<UId> peer_ids;
    
    Queue(Ctx &ctx);
    Queue(Ctx &ctx, const db::Rec<Queue> &rec);   
    Queue(const Msg &msg);
  };

  opt<Queue> find_queue_id(Ctx &ctx, UId id);
  Queue get_queue_id(Ctx &ctx, UId id);
}

#endif