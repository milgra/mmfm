#ifndef ui_filter_popup_h
#define ui_filter_popup_h

#include "view.c"

void ui_filter_popup_attach(view_t* baseview);
void ui_filter_popup_detach();
void ui_filter_popup_show();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "callbacks.c"
#include "config.c"
#include "database.c"
#include "textlist.c"
#include "tg_text.c"
#include "ui_filter_bar.c"
#include "ui_popup_switcher.c"
#include "ui_songlist.c"
#include "visible.c"
#include "zc_cstring.c"

void ui_filter_popup_on_genre_select(int index);
void ui_filter_popup_on_artist_select(int index);

struct _ui_filter_popup_t
{
  textlist_t* artistlist;
  textlist_t* genrelist;

} ufp = {0};

void ui_filter_popup_attach(view_t* baseview)
{
  textstyle_t ts = {0};
  ts.font        = config_get("font_path");
  ts.size        = 30.0;
  ts.textcolor   = 0x000000FF;
  ts.backcolor   = 0;
  ts.align       = TA_RIGHT;

  ts.margin_right = 20;
  ufp.genrelist   = textlist_new(view_get_subview(baseview, "genrelist"), ts, ui_filter_popup_on_genre_select); // REL 0
  ts.margin_left  = 20;
  ts.align        = TA_LEFT;
  ufp.artistlist  = textlist_new(view_get_subview(baseview, "artistlist"), ts, ui_filter_popup_on_artist_select); // REL 1
}

void ui_filter_popup_detach()
{
  REL(ufp.genrelist);  // REL 0
  REL(ufp.artistlist); // REL 1
}

void ui_filter_popup_show()
{
  vec_t* genres  = VNEW(); // REL 0
  vec_t* artists = VNEW(); // REL 1

  db_get_genres(genres);
  db_get_artists(artists);

  textlist_set_datasource(ufp.genrelist, genres);
  textlist_set_datasource(ufp.artistlist, artists);

  textlist_update(ufp.genrelist);
  textlist_update(ufp.artistlist);

  REL(genres);  // REL 0
  REL(artists); // REL 1

  ui_popup_switcher_toggle("filters_popup_page");
}

void ui_filter_popup_on_genre_select(int index)
{
  printf("on genre select %i\n", index);

  vec_t* genres = VNEW(); // REL 0

  db_get_genres(genres);

  char* genre = genres->data[index];

  char* query = cstr_new_format(100, "genre is %s", genre); // REL 1

  visible_set_filter(query);
  ui_songlist_update();
  ui_filter_bar_show_query(genre);

  REL(genres); // REL 0
  REL(query);  // REL 1
}

void ui_filter_popup_on_artist_select(int index)
{
  printf("on artist select %i\n", index);

  vec_t* artists = VNEW(); // REL 0

  db_get_artists(artists);

  char* artist = artists->data[index];

  char* query = cstr_new_format(100, "artist is %s", artist); // REL 1

  visible_set_filter(query);
  ui_songlist_update();
  ui_filter_bar_show_query(artist);

  REL(artists); // REL 0
  REL(query);   // REL 1
}

#endif
