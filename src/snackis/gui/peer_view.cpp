#include "snackis/ctx.hpp"
#include "snackis/gui/gui.hpp"
#include "snackis/gui/peer_view.hpp"
#include "snackis/gui/post_search.hpp"

namespace snackis {
namespace gui {
  static void on_find_posts(gpointer *_, PeerView *v) {
    PostSearch *ps = new PostSearch(v->ctx);
    select<Peer>(ps->peer_fld, v->rec);
    push_view(ps);
  }
  
  PeerView::PeerView(const Peer &pr):
    RecView<Peer>("Peer", pr),
    find_posts_btn(gtk_button_new_with_mnemonic("_Find Posts")),
    created_at_fld(gtk_entry_new()),
    changed_at_fld(gtk_entry_new()),
    name_fld(gtk_entry_new()),
    active_fld(gtk_check_button_new_with_label("Active")),
    tags_fld(gtk_entry_new()),
    email_fld(gtk_entry_new()),
    info_fld(new_text_view())
  {
    g_signal_connect(find_posts_btn, "clicked", G_CALLBACK(on_find_posts), this);
    gtk_container_add(GTK_CONTAINER(menu), find_posts_btn);
    
    GtkWidget *frm(new_grid());
    gtk_widget_set_halign(frm, GTK_ALIGN_END);    
    gtk_container_add(GTK_CONTAINER(fields), frm);

    gtk_grid_attach(GTK_GRID(frm), new_label("Created At"), 0, 0, 1, 1);
    gtk_widget_set_sensitive(created_at_fld, false);
    gtk_grid_attach(GTK_GRID(frm), created_at_fld, 0, 1, 1, 1);
    set_str(GTK_ENTRY(created_at_fld), fmt(pr.created_at, "%a %b %d, %H:%M"));

    gtk_grid_attach(GTK_GRID(frm), new_label("Changed At"), 1, 0, 1, 1);    
    gtk_widget_set_sensitive(changed_at_fld, false);
    gtk_grid_attach(GTK_GRID(frm), changed_at_fld, 1, 1, 1, 1);    
    set_str(GTK_ENTRY(changed_at_fld), fmt(pr.changed_at, "%a %b %d, %H:%M"));

    frm = new_grid();
    gtk_grid_set_column_homogeneous(GTK_GRID(frm), true);
    gtk_container_add(GTK_CONTAINER(fields), frm);
    int row(0);
    
    gtk_widget_set_halign(active_fld, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(frm), active_fld, 1, row, 1, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_fld), rec.active);

    row++;
    gtk_grid_attach(GTK_GRID(frm), new_label("Name"), 0, row, 1, 1);
    gtk_widget_set_hexpand(name_fld, true);
    gtk_grid_attach(GTK_GRID(frm), name_fld, 0, row+1, 1, 1);
    set_str(GTK_ENTRY(name_fld), pr.name);

    gtk_grid_attach(GTK_GRID(frm), new_label("Email"), 1, row, 1, 1);
    gtk_widget_set_hexpand(email_fld, true);
    gtk_grid_attach(GTK_GRID(frm), email_fld, 1, row+1, 1, 1);    
    set_str(GTK_ENTRY(email_fld), pr.email);

    row += 2;
    gtk_grid_attach(GTK_GRID(frm), new_label("Tags"), 0, row, 2, 1);
    gtk_widget_set_hexpand(tags_fld, true);
    gtk_grid_attach(GTK_GRID(frm), tags_fld, 0, row+1, 2, 1);    
    gtk_entry_set_text(GTK_ENTRY(tags_fld),
		       join(rec.tags.begin(), rec.tags.end(), ' ').c_str());

    row += 2;
    gtk_grid_attach(GTK_GRID(frm), new_label("Info"), 0, row, 2, 1);
    gtk_grid_attach(GTK_GRID(frm), gtk_widget_get_parent(info_fld), 0, row+1, 2, 1);
    set_str(GTK_TEXT_VIEW(info_fld), rec.info);
    
    focused = name_fld;
  }

  bool PeerView::save() {
    rec.name = get_str(GTK_ENTRY(name_fld));
    rec.active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(active_fld));
    rec.tags = word_set(get_str(GTK_ENTRY(tags_fld)));
    rec.email = get_str(GTK_ENTRY(email_fld));
    rec.info = get_str(GTK_TEXT_VIEW(info_fld));
    db::upsert(ctx.db.peers, rec);
    return true;
  }
}}
