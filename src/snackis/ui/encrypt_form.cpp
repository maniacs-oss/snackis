#include "snackis/ctx.hpp"
#include "snackis/db/ctx.hpp"
#include "snackis/ui/encrypt_form.hpp"
#include "snackis/ui/view.hpp"
#include "snackis/ui/window.hpp"

namespace snackis {
namespace ui {
  EncryptForm::EncryptForm(View &view, Footer &ftr):
    ViewForm(view, ftr),
    peer_name(*this, Dim(1, 50), "Peer Name"),
    peer_email(*this, Dim(1, 50), "Peer Email"),
    load_from(*this, Dim(1, 50), "Load From"),
    save_to(*this, Dim(1, 50), "Save To"),
    encode(*this, Dim(1, 5), "Encode") {
    label = "Encrypt";
    status = "Press Ctrl-s to encrypt, or Ctrl-q to cancel";
    margin_top = 1;
    
    for (auto p: ctx.db.peers.recs) {
      auto id(*get(p, ctx.db.peer_id));
      insert(peer_name, *get(p, ctx.db.peer_name), id);
      insert(peer_email, *get(p, ctx.db.peer_email), id);
    }

    peer_name.on_select = [this]() {
      auto &sel(peer_name.selected);
      assert(sel);
      select(peer_email, sel->val, false);
    };

    peer_email.on_select = [this]() {
      auto &sel(peer_email.selected);
      assert(sel);
      select(peer_name, sel->val, false);
    };

    load_from.margin_top = 1;
    for (auto &i: PathIter(*get_val(ctx.settings.load_folder))) {
      const Path p(i);
      insert(load_from, p.filename(), p);
    }
    
    load_from.on_select = [this]() {
      set_str(save_to, load_from.selected->lbl);
    };

    insert(encode, "yes", true);
    insert(encode, "no", false);
  }

  bool run(EncryptForm &frm) {
    Ctx &ctx(frm.window.ctx);

    const str ldf(*get_val(ctx.settings.load_folder));

    if (!path_exists(ldf)) {
      create_path(ldf);
      log(ctx, fmt("Nothing to load"));
      return false;
    }
    
    select(frm.encode, false);
    db::Trans trans(ctx);
    
    while (true) {
      chtype ch = get_key(frm.window);
      
      if (ch == KEY_CTRL('s') || (active_field(frm).ptr == frm.encode.ptr &&
				  ch == KEY_RETURN)) {
	validate(frm);
	create_path(*get_val(ctx.settings.save_folder));
	auto peer_sel(frm.peer_email.selected);
	auto load_from(frm.load_from.selected);
	const str save_to(get_str(frm.save_to));

	if (peer_sel && load_from && save_to != "") {
	  const str
	    in_path(load_from->val),
	    out_path(Path(*get_val(ctx.settings.save_folder)) / save_to);
	    
	  log(ctx, fmt("Encrypting from '%0' to '%1'...", in_path, out_path));
	  db::Rec<Peer> peer_rec;
	  set(peer_rec, ctx.db.peer_id, peer_sel->val);
	  load(ctx.db.peers, peer_rec);
	  Peer peer(ctx.db.peers, peer_rec);
	  encrypt(peer, in_path, out_path, frm.encode.selected->val);	    
	  db::commit(trans);
	  log(ctx, "Done encrypting");
	  break;
	}
      }

      switch (ch) {
      case KEY_CTRL('q'):
	return false;
      default:
	drive(frm, ch);
      }
    }

    return true;
  }
}}