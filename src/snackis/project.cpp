#include <iostream>
#include "snackis/ctx.hpp"
#include "snackis/peer.hpp"
#include "snackis/project.hpp"

namespace snackis {
  Project::Project(Ctx &ctx):
    IdRec(ctx), owner_id(whoami(ctx).id), active(true)
  { }

  Project::Project(Ctx &ctx, const db::Rec<Project> &rec): IdRec(ctx, null_uid) {
    copy(*this, rec);
  }

  Project::Project(const Msg &msg):
    IdRec(msg.ctx, null_uid),
    //id(msg.project_id),
    owner_id(msg.from_id),
    //name(msg.project_name),
    //info(msg.project_info),
    active(true)
  {
    peer_ids.insert(msg.from_id);

    for (auto &id: msg.peer_ids) {
      if (find_peer_id(ctx, id)) { peer_ids.insert(id); }
    }
  }

  opt<Project> find_project_id(Ctx &ctx, UId id) {
    db::Rec<Project> rec;
    set(rec, ctx.db.project_id, id);
    if (!load(ctx.db.projects, rec)) { return nullopt; }
    return Project(ctx, rec);
  }

  Project get_project_id(Ctx &ctx, UId id) {
    auto found(find_project_id(ctx, id));
    
    if (!found) {
      ERROR(Db, fmt("Project id not found: %0", id));
    }

    return *found;
  }

  Feed get_feed(const Project &prj) {
    Ctx &ctx(prj.ctx);
    db::Rec<Feed> rec;
    db::set(rec, ctx.db.feed_id, prj.id);
    Feed feed(ctx, rec);

    if (!db::load(ctx.db.feeds, feed)) {
      feed.name = fmt("Project %0", id_str(prj));
      feed.visible = false;
      feed.peer_ids = prj.peer_ids;
      db::insert(ctx.db.feeds, feed);
    }
    
    return feed;
  }
}
