#include "snackis/ctx.hpp"
#include "snackis/snackis.hpp"
#include "snackis/gui/gui.hpp"
#include "snackis/gui/login.hpp"
#include "snackis/gui/reader.hpp"

namespace snackis {
namespace gui {
  static void on_login(gpointer *_, Login *v) {
    const str pass(gtk_entry_get_text(GTK_ENTRY(v->pass)));

    if (v->repeat) {  
      if (pass != gtk_entry_get_text(GTK_ENTRY(v->repeat))) {
	gtk_widget_grab_focus(v->pass);
	return;
      }
      
      db::init_pass(v->ctx, pass);
    } if (!db::login(v->ctx, pass)) {
	gtk_widget_grab_focus(v->pass);
	return;
    }

    Ctx &ctx(v->ctx);
    
    if (!reader) {
      open(ctx);
      imap_worker.emplace(ctx);
      smtp_worker.emplace(ctx);

      reader.emplace(ctx);
      gtk_box_pack_start(GTK_BOX(left_panel), reader->ptr(), false, false, 5);
    }

    Peer me(whoami(ctx));
    if (!me.name.empty()) { log(ctx, fmt("Welcome back, %0", me.name)); }
    pop_view(v);
    delete v;
    gtk_widget_show_all(gui::window);
  }
  
  Login::Login(Ctx &ctx):
    View(ctx, ""),
    pass(gtk_entry_new()),
    repeat(nullptr)
  {
    GtkWidget *frm = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(panel), frm, true, true, 0);
    gtk_widget_set_halign(frm, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(frm, GTK_ALIGN_CENTER);
    
    GtkWidget *logo = gtk_image_new_from_file("logo.png");
    gtk_widget_set_halign(logo, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(frm), logo);

    gtk_box_pack_start(GTK_BOX(frm), new_label("Password"), false, false, 5);

    gtk_entry_set_visibility(GTK_ENTRY(pass), false);
    gtk_entry_set_width_chars(GTK_ENTRY(pass), 50);
    g_signal_connect(pass, "activate", G_CALLBACK(on_login), this);
    gtk_box_pack_start(GTK_BOX(frm), pass, false, false, 0);

    if (!pass_exists(ctx)) {
      gtk_box_pack_start(GTK_BOX(frm), new_label("Repeat"), false, false, 5);      
      repeat = gtk_entry_new();
      gtk_entry_set_visibility(GTK_ENTRY(repeat), false);
      gtk_entry_set_width_chars(GTK_ENTRY(repeat), 50);
      g_signal_connect(repeat, "activate", G_CALLBACK(on_login), this);
      gtk_box_pack_start(GTK_BOX(frm), repeat, false, false, 0);
    }
    
    btn = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(btn), "Login");
    gtk_box_pack_start(GTK_BOX(frm), btn, false, false, 10);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_login), this);
    gtk_widget_set_halign(btn, GTK_ALIGN_CENTER);
    focused = pass;
  }
}}
