#include "snackis/ctx.hpp"
#include "snackis/gui/gui.hpp"
#include "snackis/gui/setup.hpp"
#include "snackis/net/imap.hpp"
#include "snackis/net/smtp.hpp"

namespace snackis {
namespace gui {
  static void on_load_folder(gpointer *_, Setup *setup) {
    GtkWidget *dlg(gtk_file_chooser_dialog_new("Select Load Folder",
					       GTK_WINDOW(window),
					       GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
					       "_Cancel", GTK_RESPONSE_CANCEL,
					       "_Select", GTK_RESPONSE_ACCEPT,
					       nullptr));
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg),
				  gtk_entry_get_text(GTK_ENTRY(setup->load_folder)));

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
      char *dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
      gtk_entry_set_text(GTK_ENTRY(setup->load_folder), dir);
      g_free(dir);
      gtk_widget_destroy(dlg);
    }
  }

  static void on_save_folder(gpointer *_, Setup *setup) {
    GtkWidget *dlg(gtk_file_chooser_dialog_new("Select Save Folder",
					       GTK_WINDOW(window),
					       GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
					       "_Cancel", GTK_RESPONSE_CANCEL,
					       "_Select", GTK_RESPONSE_ACCEPT,
					       nullptr));
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg),
				  gtk_entry_get_text(GTK_ENTRY(setup->save_folder)));

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
      char *dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
      gtk_entry_set_text(GTK_ENTRY(setup->save_folder), dir);
      g_free(dir);
      gtk_widget_destroy(dlg);
    }
  }

  static void on_cancel(gpointer *_, Setup *setup) {
    log(setup->ctx, "Cancelled setup");
    setup->pop_view();
  }

  static void copy_imap(Setup &setup) {
    Ctx &ctx(setup.ctx);
    set_val(ctx.settings.imap_url,
	    str(gtk_entry_get_text(GTK_ENTRY(setup.imap_url))));

    const str port_str(gtk_entry_get_text(GTK_ENTRY(setup.imap_port)));
    auto port(to_int64(port_str));
    
    if (!port) {
      log(ctx, fmt("Invalid Imap port: %0", port_str));
    } else {
      set_val(ctx.settings.imap_port, *port);
    }

    set_val(ctx.settings.imap_user,
	    str(gtk_entry_get_text(GTK_ENTRY(setup.imap_user))));
    set_val(ctx.settings.imap_pass,
	    str(gtk_entry_get_text(GTK_ENTRY(setup.imap_pass))));

    const str poll_str(gtk_entry_get_text(GTK_ENTRY(setup.imap_poll)));
    auto poll(to_int64(poll_str));
    
    if (!poll) {
      log(ctx, fmt("Invalid Imap poll: %0", poll_str));
    } else {
      set_val(ctx.settings.imap_poll, *poll);
    }
  }

  static void copy_smtp(Setup &setup) {
    Ctx &ctx(setup.ctx);
    set_val(ctx.settings.smtp_url,
	    str(gtk_entry_get_text(GTK_ENTRY(setup.smtp_url))));

    const str port_str(gtk_entry_get_text(GTK_ENTRY(setup.smtp_port)));
    auto port(to_int64(port_str));
    
    if (!port) {
      log(ctx, fmt("Invalid Smtp port: %0", port_str));
    } else {
      set_val(ctx.settings.smtp_port, *port);
    }

    set_val(ctx.settings.smtp_user,
	    str(gtk_entry_get_text(GTK_ENTRY(setup.smtp_user))));
    set_val(ctx.settings.smtp_pass,
	    str(gtk_entry_get_text(GTK_ENTRY(setup.smtp_pass))));

    const str poll_str(gtk_entry_get_text(GTK_ENTRY(setup.smtp_poll)));
    auto poll(to_int64(poll_str));
    
    if (!poll) {
      log(ctx, fmt("Invalid Smtp poll: %0", poll_str));
    } else {
      set_val(ctx.settings.smtp_poll, *poll);
    }
  }

  static void on_save(gpointer *_, Setup *setup) {
    Ctx &ctx(setup->ctx);
    
    db::Trans trans(ctx);
    Peer &me(whoami(ctx));
    me.name = gtk_entry_get_text(GTK_ENTRY(setup->name));
    me.email = gtk_entry_get_text(GTK_ENTRY(setup->email));
    update(ctx.db.peers, me);

    set_val(ctx.settings.load_folder,
	    str(gtk_entry_get_text(GTK_ENTRY(setup->load_folder))));
    set_val(ctx.settings.save_folder,
	    str(gtk_entry_get_text(GTK_ENTRY(setup->save_folder))));

    copy_imap(*setup);
    if (*get_val(ctx.settings.imap_poll)) {
      ctx.fetch_cond.notify_one();
    }
    
    copy_smtp(*setup);
    if (*get_val(ctx.settings.smtp_poll)) {
      ctx.send_cond.notify_one();
    }

    db::commit(trans);
    log(setup->ctx, "Saved setup");
    setup->pop_view();
  }
  
  static void on_imap(gpointer *_, Setup *setup) {
    Ctx &ctx(setup->ctx);
    db::Trans trans(setup->ctx);
    copy_imap(*setup);

    try {
      Imap imap(ctx);
      log(ctx, "Imap Ok");
    } catch (const ImapError &e) {
      log(ctx, e.what());
    }
  }

  static void on_smtp(gpointer *_, Setup *setup) {
    Ctx &ctx(setup->ctx);
    db::Trans trans(setup->ctx);
    copy_smtp(*setup);

    try {
      Smtp smtp(ctx);
      log(ctx, "Smtp Ok");
    } catch (const SmtpError &e) {
      log(ctx, e.what());
    }
  }

  static GtkWidget *init_load_folder(Setup &setup) {
    Ctx &ctx(setup.ctx);
    GtkWidget *frm = gtk_grid_new();
    gtk_widget_set_margin_top(frm, 5);
    gtk_grid_set_row_spacing(GTK_GRID(frm), 5);
    gtk_grid_set_column_spacing(GTK_GRID(frm), 5);
    GtkWidget *lbl = gtk_label_new("Load Folder");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 0, 0, 1, 1);
    gtk_widget_set_hexpand(setup.load_folder, true);
    gtk_editable_set_editable(GTK_EDITABLE(setup.load_folder), false);
    gtk_widget_set_can_focus(setup.load_folder, false);
    gtk_entry_set_text(GTK_ENTRY(setup.load_folder),
		       get_val(ctx.settings.load_folder)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.load_folder, 0, 1, 1, 1);

    GtkWidget *btn = gtk_button_new_with_label("Select Folder");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_load_folder), &setup);
    gtk_grid_attach(GTK_GRID(frm), btn, 1, 1, 1, 1);
    return frm;
  }

  static GtkWidget *init_save_folder(Setup &setup) {
    Ctx &ctx(setup.ctx);
    GtkWidget *frm = gtk_grid_new();
    gtk_widget_set_margin_top(frm, 5);
    gtk_grid_set_row_spacing(GTK_GRID(frm), 5);
    gtk_grid_set_column_spacing(GTK_GRID(frm), 5);
    GtkWidget *lbl = gtk_label_new("Save Folder");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 0, 0, 1, 1);
    gtk_widget_set_hexpand(setup.save_folder, true);
    gtk_editable_set_editable(GTK_EDITABLE(setup.save_folder), false);
    gtk_widget_set_can_focus(setup.save_folder, false);
    gtk_entry_set_text(GTK_ENTRY(setup.save_folder),
		       get_val(ctx.settings.save_folder)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.save_folder, 0, 1, 1, 1);

    GtkWidget *btn = gtk_button_new_with_label("Select Folder");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_save_folder), &setup);
    gtk_grid_attach(GTK_GRID(frm), btn, 1, 1, 1, 1);
    return frm;
  }

  static GtkWidget *init_imap(Setup &setup) {
    Ctx &ctx(setup.ctx);
    
    GtkWidget *frm = gtk_grid_new();
    gtk_widget_set_margin_top(frm, 5);
    gtk_grid_set_row_spacing(GTK_GRID(frm), 5);
    gtk_grid_set_column_spacing(GTK_GRID(frm), 5);

    int row = 0;
    GtkWidget *lbl(nullptr);
    
    lbl = gtk_label_new("Imap URL");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 0, row, 2, 1);
    gtk_widget_set_hexpand(setup.imap_url, true);
    gtk_entry_set_text(GTK_ENTRY(setup.imap_url),
		       get_val(ctx.settings.imap_url)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.imap_url, 0, row+1, 2, 1);

    lbl = gtk_label_new("Imap Port");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(frm), lbl, 2, row, 1, 1);
    gtk_entry_set_text(GTK_ENTRY(setup.imap_port),
		       to_str(*get_val(ctx.settings.imap_port)).c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.imap_port, 2, row+1, 1, 1);

    row += 2;
    lbl = gtk_label_new("Imap User");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 0, row, 1, 1);
    gtk_widget_set_hexpand(setup.imap_user, true);
    gtk_entry_set_text(GTK_ENTRY(setup.imap_user),
		       get_val(ctx.settings.imap_user)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.imap_user, 0, row+1, 1, 1);

    lbl = gtk_label_new("Imap Password");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 1, row, 1, 1);
    gtk_entry_set_visibility(GTK_ENTRY(setup.imap_pass), false);
    gtk_widget_set_hexpand(setup.imap_pass, true);
    gtk_entry_set_text(GTK_ENTRY(setup.imap_pass),
		       get_val(ctx.settings.imap_pass)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.imap_pass, 1, row+1, 1, 1);

    lbl = gtk_label_new("Imap Poll (s)");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(frm), lbl, 2, row, 1, 1);
    gtk_entry_set_text(GTK_ENTRY(setup.imap_poll),
		       to_str(*get_val(ctx.settings.imap_poll)).c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.imap_poll, 2, row+1, 1, 1);

    row += 2;
    GtkWidget *btn = gtk_button_new_with_label("Test Imap");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_imap), &setup);
    gtk_grid_attach(GTK_GRID(frm), btn, 2, row, 1, 1);
    return frm;
  }

  static GtkWidget *init_smtp(Setup &setup) {
    Ctx &ctx(setup.ctx);
    
    GtkWidget *frm = gtk_grid_new();
    gtk_widget_set_margin_top(frm, 5);
    gtk_grid_set_row_spacing(GTK_GRID(frm), 5);
    gtk_grid_set_column_spacing(GTK_GRID(frm), 5);

    int row = 0;
    GtkWidget *lbl(nullptr);
    
    lbl = gtk_label_new("Smtp URL");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 0, row, 2, 1);
    gtk_widget_set_hexpand(setup.smtp_url, true);
    gtk_entry_set_text(GTK_ENTRY(setup.smtp_url),
		       get_val(ctx.settings.smtp_url)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.smtp_url, 0, row+1, 2, 1);

    lbl = gtk_label_new("Smtp Port");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(frm), lbl, 2, row, 1, 1);
    gtk_entry_set_text(GTK_ENTRY(setup.smtp_port),
		       to_str(*get_val(ctx.settings.smtp_port)).c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.smtp_port, 2, row+1, 1, 1);

    row += 2;
    lbl = gtk_label_new("Smtp User");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 0, row, 1, 1);
    gtk_widget_set_hexpand(setup.smtp_user, true);
    gtk_entry_set_text(GTK_ENTRY(setup.smtp_user),
		       get_val(ctx.settings.smtp_user)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.smtp_user, 0, row+1, 1, 1);

    lbl = gtk_label_new("Smtp Password");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_grid_attach(GTK_GRID(frm), lbl, 1, row, 1, 1);
    gtk_entry_set_visibility(GTK_ENTRY(setup.smtp_pass), false);
    gtk_widget_set_hexpand(setup.smtp_pass, true);
    gtk_entry_set_text(GTK_ENTRY(setup.smtp_pass),
		       get_val(ctx.settings.smtp_pass)->c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.smtp_pass, 1, row+1, 1, 1);

    lbl = gtk_label_new("Smtp Poll (s)");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(frm), lbl, 2, row, 1, 1);
    gtk_entry_set_text(GTK_ENTRY(setup.smtp_poll),
		       to_str(*get_val(ctx.settings.smtp_poll)).c_str());
    gtk_grid_attach(GTK_GRID(frm), setup.smtp_poll, 2, row+1, 1, 1);

    row += 2;
    GtkWidget *btn = gtk_button_new_with_label("Test Smtp");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_smtp), &setup);
    gtk_grid_attach(GTK_GRID(frm), btn, 2, row, 1, 1);
    return frm;
  }

  Setup::Setup(Ctx &ctx):
    View(ctx, "Setup"),
    name(gtk_entry_new()),
    email(gtk_entry_new()),
    load_folder(gtk_entry_new()),
    save_folder(gtk_entry_new()),
    imap_url(gtk_entry_new()),
    imap_port(gtk_entry_new()),
    imap_user(gtk_entry_new()),
    imap_pass(gtk_entry_new()),
    imap_poll(gtk_entry_new()),
    smtp_url(gtk_entry_new()),
    smtp_port(gtk_entry_new()),
    smtp_user(gtk_entry_new()),
    smtp_pass(gtk_entry_new()),
    smtp_poll(gtk_entry_new()) {
    GtkWidget *lbl;

    GtkWidget *frm = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(panel), frm, true, true, 0);
    
    Peer &me(whoami(ctx));

    lbl = gtk_label_new("Name");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(frm), lbl);
    gtk_widget_set_hexpand(name, true);
    gtk_entry_set_text(GTK_ENTRY(name), me.name.c_str());
    gtk_container_add(GTK_CONTAINER(frm), name);
    
    lbl = gtk_label_new("Email");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);  
    gtk_container_add(GTK_CONTAINER(frm), lbl);
    gtk_widget_set_hexpand(email, true);
    gtk_entry_set_text(GTK_ENTRY(email), me.email.c_str());
    gtk_container_add(GTK_CONTAINER(frm), email);

    gtk_container_add(GTK_CONTAINER(frm), init_load_folder(*this));
    gtk_container_add(GTK_CONTAINER(frm), init_save_folder(*this));

    gtk_container_add(GTK_CONTAINER(frm), init_imap(*this));
    gtk_container_add(GTK_CONTAINER(frm), init_smtp(*this));
    
    GtkWidget *btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(btns, GTK_ALIGN_END);
    gtk_widget_set_valign(btns, GTK_ALIGN_END);
    gtk_container_add(GTK_CONTAINER(panel), btns);
        
    cancel = gtk_button_new_with_mnemonic("_Cancel Changes");
    g_signal_connect(cancel, "clicked", G_CALLBACK(on_cancel), this);
    gtk_container_add(GTK_CONTAINER(btns), cancel);
    
    save = gtk_button_new_with_mnemonic("_Save Changes");
    g_signal_connect(save, "clicked", G_CALLBACK(on_save), this);
    gtk_container_add(GTK_CONTAINER(btns), save);
  }
  
  void Setup::focus() {
    gtk_widget_grab_focus(name);
  }
}}
