#include <cassert>
#include "snackis/ctx.hpp"
#include "snackis/gui/gui.hpp"
#include "snackis/gui/post_search.hpp"
#include "snackis/gui/post_view.hpp"

namespace snackis {
namespace gui {
  enum PostCol {COL_PTR=0, COL_ID, COL_BY, COL_TAGS, COL_BODY};
  
  static void edit(Ctx &ctx, const db::Rec<Post> &rec) {
    Post post(ctx, rec);    
    PostView *fv(new PostView(post));
    push_view(*fv);
  }

  PostSearch::PostSearch(Ctx &ctx):
    SearchView<Post>(ctx,
		     "Post",
		     gtk_list_store_new(5, G_TYPE_POINTER,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING),
		     [&ctx](auto &rec) { edit(ctx, rec); }),
    id_fld(new_id_field()),
    tags_fld(gtk_entry_new()),
    body_fld(gtk_entry_new()),
    min_time_fld(gtk_entry_new()),
    max_time_fld(gtk_entry_new()),
    feed_fld(ctx),
    peer_fld(ctx)
  {
    GtkWidget *lbl;

    GtkWidget *top_box(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
    gtk_container_add(GTK_CONTAINER(fields), top_box);

    GtkWidget *id_box(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
    gtk_container_add(GTK_CONTAINER(top_box), id_box);
    lbl = gtk_label_new("Id");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(id_box), lbl);
    gtk_entry_set_width_chars(GTK_ENTRY(id_fld), ID_LEN);
    gtk_entry_set_max_width_chars(GTK_ENTRY(id_fld), ID_LEN);
    gtk_container_add(GTK_CONTAINER(id_box), id_fld);
    
    GtkWidget *post_box(gtk_grid_new());
    gtk_grid_set_row_spacing(GTK_GRID(post_box), 5);
    gtk_grid_set_column_spacing(GTK_GRID(post_box), 5);
    gtk_widget_set_halign(post_box, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(top_box), post_box, true, true, 0);

    lbl = gtk_label_new("Posted");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(post_box), lbl, 0, 0, 3, 1);
    gtk_entry_set_placeholder_text(GTK_ENTRY(min_time_fld),
				   "yyyy-mm-dd hh:mm");
    gtk_grid_attach(GTK_GRID(post_box), min_time_fld, 0, 1, 1, 1);
    gtk_entry_set_text(GTK_ENTRY(min_time_fld),
		       fmt(now() - std::chrono::hours(7*24),
			   "%Y-%m-%d %H:%M").c_str());
    gtk_grid_attach(GTK_GRID(post_box), gtk_label_new("-"), 1, 1, 1, 1);
    gtk_entry_set_placeholder_text(GTK_ENTRY(max_time_fld),
				   "yyyy-mm-dd hh:mm");
    gtk_grid_attach(GTK_GRID(post_box), max_time_fld, 2, 1, 1, 1);

    GtkWidget *body_box(gtk_grid_new());
    gtk_grid_set_row_spacing(GTK_GRID(body_box), 5);
    gtk_grid_set_column_spacing(GTK_GRID(body_box), 5);
    gtk_container_add(GTK_CONTAINER(fields), body_box);
    
    lbl = gtk_label_new("Tags");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(body_box), lbl, 0, 0, 1, 1);
    gtk_widget_set_hexpand(tags_fld, true);
    gtk_grid_attach(GTK_GRID(body_box), tags_fld, 0, 1, 1, 1);

    lbl = gtk_label_new("Body");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(body_box), lbl, 1, 0, 1, 1);
    gtk_widget_set_hexpand(body_fld, true);
    gtk_grid_attach(GTK_GRID(body_box), body_fld, 1, 1, 1, 1);

    lbl = gtk_label_new("Feed");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(fields), lbl);
    gtk_container_add(GTK_CONTAINER(fields), feed_fld.ptr());
    
    lbl = gtk_label_new("By");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_container_add(GTK_CONTAINER(fields), lbl);
    gtk_container_add(GTK_CONTAINER(fields), peer_fld.ptr());
    
    add_col(GTK_TREE_VIEW(list), "Id", COL_ID);
    add_col(GTK_TREE_VIEW(list), "Posted", COL_BY);
    add_col(GTK_TREE_VIEW(list), "Tags", COL_TAGS);
    add_col(GTK_TREE_VIEW(list), "Body", COL_BODY, true);

    focused = id_fld;
  }

  void PostSearch::find() {
    size_t cnt(0);
    
    str id_sel(trim(gtk_entry_get_text(GTK_ENTRY(id_fld))));
    str tags_str(trim(gtk_entry_get_text(GTK_ENTRY(tags_fld))));
    std::set<str> tags_sel(word_set(tags_str));
    str body_sel(trim(gtk_entry_get_text(GTK_ENTRY(body_fld))));
    auto feed_sel(feed_fld.selected);
    auto peer_sel(peer_fld.selected);
    str min_time_str(trim(gtk_entry_get_text(GTK_ENTRY(min_time_fld))));
    auto min_time_sel(parse_time("%Y-%m-%d %H:%M", min_time_str));
    
    if (!min_time_str.empty() && !min_time_sel) {
      log(ctx, fmt("Failed parsing time: '%0'", min_time_str));
      gtk_widget_grab_focus(min_time_fld);
      return;
    }

    str max_time_str(trim(gtk_entry_get_text(GTK_ENTRY(max_time_fld))));
    auto max_time_sel(parse_time("%Y-%m-%d %H:%M", max_time_str));
    
    if (!max_time_str.empty() && !max_time_sel) {
      log(ctx, fmt("Failed parsing time: '%0'", max_time_str));
      gtk_widget_grab_focus(max_time_fld);
      return;
    }

    for (auto key = ctx.db.posts_sort.recs.rbegin();
	 key != ctx.db.posts_sort.recs.rend();
	 key++) {
      auto &rec(db::get(ctx.db.posts, *key));
      Post post(ctx, rec);
      Feed feed(get_feed_id(ctx, post.feed_id));

      if (!id_sel.empty() && find_ci(id_str(post), id_sel) == str::npos) {
	continue;
      }

      if (!tags_sel.empty()) {
	std::vector<str> diff;
	std::set_difference(tags_sel.begin(), tags_sel.end(),
			    post.tags.begin(), post.tags.end(),
			    std::back_inserter(diff));
	if (!diff.empty()) { continue; }
      }
      
      if (!body_sel.empty() && find_ci(post.body, body_sel) == str::npos) {
	continue;
      }

      if (!min_time_str.empty() && post.created_at < *min_time_sel) {
	continue;
      }

      if (!max_time_str.empty() && post.created_at > *max_time_sel) {
	continue;
      }
      
      if (feed_sel) {
	if (post.feed_id != feed_sel->id) { continue; }
      } else {
	if (!feed.visible) { continue; }
      }

      if (peer_sel) {
	if (post.owner_id != peer_sel->id) { continue; }
      }
      
      Peer peer(get_peer_id(ctx, post.owner_id));
      GtkTreeIter iter;
      gtk_list_store_append(store, &iter);
      const str by(fmt("%0\n%1",
		       peer.name.c_str(),
		       fmt(post.created_at, "%a %b %d, %H:%M").c_str()));
      gtk_list_store_set(store, &iter,
			 COL_PTR, &rec,
			 COL_ID, id_str(post).c_str(),
			 COL_BY, by.c_str(),
			 COL_TAGS,
			 join(post.tags.begin(), post.tags.end(), '\n').c_str(),
			 COL_BODY, post.body.c_str(),
			 -1);
      cnt++;
    }

    gtk_widget_grab_focus(cnt ? list : id_fld);
  }
}}
