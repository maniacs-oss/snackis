#include <cassert>
#include "snackis/ctx.hpp"
#include "snackis/gui/gui.hpp"
#include "snackis/gui/feed_search.hpp"
#include "snackis/gui/feed_view.hpp"

namespace snackis {
namespace gui {
  enum FeedCol {COL_PTR=0, COL_ID, COL_CREATED, COL_OWNER, COL_TAGS,
		COL_INFO};

  static void edit(Ctx &ctx, const db::Rec<Feed> &rec) {
    Feed feed(ctx, rec);
    push_view(new FeedView(feed));
  }
  
  FeedSearch::FeedSearch(Ctx &ctx):
    SearchView<Feed>(ctx,
		     "Feed",
		     gtk_list_store_new(6,
					G_TYPE_POINTER,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING),
		     [&ctx](auto &rec) { edit(ctx, rec); }),
    id_fld(new_id_field()),
    active_fld(gtk_check_button_new_with_label("Active")),
    tags_fld(gtk_entry_new()),
    text_fld(gtk_entry_new()),
    peer_fld(ctx)
  {
    GtkWidget *top_box(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
    gtk_container_add(GTK_CONTAINER(fields), top_box);

    GtkWidget *id_box(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
    gtk_container_add(GTK_CONTAINER(top_box), id_box);

    gtk_container_add(GTK_CONTAINER(id_box), new_label("Id"));
    gtk_container_add(GTK_CONTAINER(id_box), id_fld);
    gtk_widget_set_halign(active_fld, GTK_ALIGN_END);
    gtk_widget_set_valign(active_fld, GTK_ALIGN_END);
    gtk_widget_set_hexpand(active_fld, true);
    gtk_container_add(GTK_CONTAINER(top_box), active_fld);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_fld), true);
    
    auto frm(new_grid());
    gtk_grid_set_column_homogeneous(GTK_GRID(frm), true);
    gtk_container_add(GTK_CONTAINER(fields), frm);

    int row(0);
    gtk_grid_attach(GTK_GRID(frm), new_label("Tags"), 0, row, 1, 1);
    gtk_widget_set_hexpand(tags_fld, true);
    gtk_grid_attach(GTK_GRID(frm), tags_fld, 0, row+1, 1, 1);

    gtk_grid_attach(GTK_GRID(frm), new_label("Text"), 1, row, 1, 1);
    gtk_widget_set_hexpand(text_fld, true);
    gtk_grid_attach(GTK_GRID(frm), text_fld, 1, row+1, 1, 1);

    row += 2;
    gtk_grid_attach(GTK_GRID(frm), new_label("Owner"), 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(frm), peer_fld.ptr(), 0, row+1, 1, 1);

    add_col(GTK_TREE_VIEW(list), "Id", COL_ID);
    add_col(GTK_TREE_VIEW(list), "Created", COL_CREATED);
    add_col(GTK_TREE_VIEW(list), "Owner", COL_OWNER);
    add_col(GTK_TREE_VIEW(list), "Tags", COL_TAGS);
    add_col(GTK_TREE_VIEW(list), "Info", COL_INFO, true);

    focused = id_fld;
  }

  void FeedSearch::find() {
    size_t cnt(0);
    
    str id_sel(trim(gtk_entry_get_text(GTK_ENTRY(id_fld))));
    bool active_sel(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(active_fld)));
    str tags_str(trim(gtk_entry_get_text(GTK_ENTRY(tags_fld))));
    std::set<str> tags_sel(word_set(tags_str));
    str text_sel(trim(gtk_entry_get_text(GTK_ENTRY(text_fld))));
    auto &peer_sel(peer_fld.selected);
    
    for (auto key = ctx.db.feeds_sort.recs.rbegin();
	 key != ctx.db.feeds_sort.recs.rend();
	 key++) {
      auto &rec(db::get(ctx.db.feeds, key->second));
      Feed feed(ctx, rec);

      if (id_sel.empty() && !feed.visible) { continue; }

      if (!id_sel.empty() && find_ci(id_str(feed), id_sel) == str::npos) {
	continue;
      }

      if (feed.active != active_sel) { continue; }

      if (!tags_sel.empty()) {
	std::vector<str> diff;
	std::set_difference(tags_sel.begin(), tags_sel.end(),
			    feed.tags.begin(), feed.tags.end(),
			    std::back_inserter(diff));
	if (!diff.empty()) { continue; }
      }
      
      if (!text_sel.empty() &&
	  find_ci(feed.name, text_sel) == str::npos &&
	  find_ci(feed.info, text_sel) == str::npos) {
	continue;
      }

      if (peer_sel) {
	if (feed.owner_id != peer_sel->id &&
	    feed.peer_ids.find(peer_sel->id) == feed.peer_ids.end()) { continue; }
      }      
      
      Peer own(get_peer_id(ctx, feed.owner_id));

      GtkTreeIter iter;
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter,
			 COL_PTR, &rec,
			 COL_ID, id_str(feed).c_str(),
			 COL_CREATED,
			 fmt(feed.created_at, "%a %b %d, %H:%M").c_str(),
			 COL_OWNER, own.name.c_str(),
			 COL_TAGS,
			 join(feed.tags.begin(), feed.tags.end(), '\n').c_str(),
			 COL_INFO, trim(fmt("%0\n%1", feed.name, feed.info)).c_str(),
			 -1);
      cnt++;
    }

    gtk_widget_grab_focus(cnt ? list : id_fld);
  }
}}
