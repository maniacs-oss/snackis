#include "snackis/ctx.hpp"
#include "snackis/invite.hpp"
#include "snackis/msg.hpp"

namespace snackis {
  Invite::Invite(Ctx &ctx, const str &to): Rec(ctx), to(to)
  { }

  Invite::Invite(Ctx &ctx, const db::Rec<Invite> &src): Rec(ctx) {
    copy(*this, src);
  }

  void send(Invite &inv) {
    Ctx &ctx(inv.ctx);
    inv.posted_at = now();
    upsert(ctx.db.invites, inv);
    Msg msg(ctx, Msg::INVITE);
    msg.to = inv.to;
    insert(inv.ctx.db.outbox, msg);
  }
  
  void send_accept(const Msg &in) {
    Ctx &ctx(in.ctx);
    Peer pr(in);
    db::upsert(ctx.db.peers, pr);
    Msg out(ctx, Msg::ACCEPT);
    out.to = in.from;
    out.to_id = in.from_id;
    db::insert(ctx.db.outbox, out);
  }

  bool invite_accepted(const Msg &in) {
    Ctx &ctx(in.ctx);
    Peer peer(in);
    db::upsert(ctx.db.peers, peer);

    db::Rec<Invite> inv;
    set(inv, ctx.db.invite_to, in.from);
    return erase(ctx.db.invites, inv) > 0;
  }

  void send_reject(const Msg &in) {
    Ctx &ctx(in.ctx);
    Msg out(ctx, Msg::REJECT);
    out.to = in.from;
    insert(ctx.db.outbox, out);
  }

  bool invite_rejected(const Msg &in) {
    Ctx &ctx(in.ctx);
    db::Rec<Invite> inv;
    set(inv, ctx.db.invite_to, in.from);
    return erase(ctx.db.invites, inv) > 0;
  }
}
