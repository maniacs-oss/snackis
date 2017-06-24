#ifndef SNACKIS_PEER_HPP
#define SNACKIS_PEER_HPP

#include "snackis/rec.hpp"
#include "snackis/core/path.hpp"
#include "snackis/core/str.hpp"
#include "snackis/core/time.hpp"
#include "snackis/core/uid.hpp"
#include "snackis/crypt/pub_key.hpp"
#include "snackis/db/rec.hpp"

namespace snackis {
  struct Peer: Rec {
    UId id;
    str name, email;
    crypt::PubKey crypt_key;
    
    Peer(Ctx &ctx);
    Peer(Ctx &ctx, const db::Rec<Peer> &src);
  };

  opt<Peer> find_peer_id(Ctx &ctx, const UId &id);
  Peer get_peer_id(Ctx &ctx, const UId &id);
  void encrypt(const Peer &peer, const Path &in, const Path &out, bool encode);
  void decrypt(const Peer &peer, const Path &in, const Path &out, bool encode);
}

#endif
