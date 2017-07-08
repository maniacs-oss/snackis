#include <iostream>
#include "snackis/ctx.hpp"
#include "snackis/peer.hpp"
#include "snackis/project.hpp"
#include "snackis/task.hpp"

namespace snackis {
  Task::Task(Ctx &ctx):
    IdRec(ctx),
    owner_id(whoami(ctx).id),
    created_at(now()),
    changed_at(created_at),
    done(false)
  { }

  Task::Task(Ctx &ctx, const db::Rec<Task> &rec): IdRec(ctx, false) {
    db::copy(*this, rec);
    auto prj(find_project_id(ctx, project_id));
    if (prj) { peer_ids = prj->peer_ids; }
  }

  Task::Task(const Msg &msg):
    IdRec(msg.ctx, *db::get(msg.task, msg.ctx.db.task_id)),
    created_at(now()),
    changed_at(created_at)
  {
    copy(*this, msg);
  }

  void copy(Task &dst, const Msg &src) {
    Ctx &ctx(src.ctx);
    const Task tsk(get_task_id(ctx, *db::get(src.task, ctx.db.task_id)));

    dst.id = tsk.id;
    dst.project_id = tsk.project_id;
    dst.owner_id = tsk.owner_id;
    dst.name = tsk.name;
    dst.info = tsk.info;
    dst.done = tsk.done;

    auto my_pid(whoami(ctx).id);
    std::copy_if(tsk.peer_ids.begin(), tsk.peer_ids.end(),
		 std::inserter(dst.peer_ids, dst.peer_ids.end()),
		 [&ctx, &my_pid](auto &pid) {
		   return find_peer_id(ctx, pid) && pid != my_pid;
		 });
    dst.peer_ids.insert(src.from_id);
  }
  
  opt<Task> find_task_id(Ctx &ctx, UId id) {
    db::Rec<Task> rec;
    set(rec, ctx.db.task_id, id);
    if (!load(ctx.db.tasks, rec)) { return nullopt; }
    return Task(ctx, rec);
  }

  Task get_task_id(Ctx &ctx, UId id) {
    auto found(find_task_id(ctx, id));
    
    if (!found) {
      ERROR(Db, fmt("Task id not found: %0", id));
    }

    return *found;
  }

  Feed get_feed(const Task &tsk) {
    Ctx &ctx(tsk.ctx);
    db::Rec<Feed> rec;
    db::set(rec, ctx.db.feed_id, tsk.id);
    Feed fd(ctx, rec);

    if (!db::load(ctx.db.feeds, fd)) {
      db::Trans trans(ctx);
      TRY(try_create);
      fd.name = fmt("Task %0", id_str(tsk));
      fd.owner_id = tsk.owner_id;
      fd.active = true;
      fd.visible = false;
      fd.peer_ids = tsk.peer_ids;
      db::insert(ctx.db.feeds, fd);
      if (try_create.errors.empty()) { db::commit(trans); }
    }
    
    return fd;
  }

  void set_project(Task &tsk, Project &prj) {
    tsk.project_id = prj.id;
    tsk.peer_ids = prj.peer_ids;
  }

  static void send(const Task &tsk, const Peer &pr) {
    Ctx &ctx(tsk.ctx);
    Msg msg(ctx, Msg::TASK);
    msg.to = pr.email;
    msg.to_id = pr.id;
    db::copy(ctx.db.projects, msg.project, get_project_id(ctx, tsk.project_id));
    db::copy(ctx.db.tasks, msg.task, tsk);
    insert(ctx.db.outbox, msg);
  }

  void send(const Task &tsk) {
    Ctx &ctx(tsk.ctx);
    if (tsk.owner_id == whoami(ctx).id) {
      for (auto &pid: tsk.peer_ids) {
	auto pr(find_peer_id(ctx, pid));
	if (pr) { send(tsk, *pr); }
      }
    } else {
      send(tsk, get_peer_id(ctx, tsk.owner_id));
    }
  }
}
