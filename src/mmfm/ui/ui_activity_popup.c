#ifndef ui_activity_popup_h
#define ui_activity_popup_h

#include "view.c"
#include "zc_text.c"

void ui_activity_popup_attach(view_t* baseview);
void ui_activity_popup_detach();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "textlist.c"
#include "tg_text.c"
#include "zc_log.c"
#include <pthread.h>

struct _ui_activity_popup_t
{
  vec_t*          logs;
  int             ind;
  textlist_t*     list;
  view_t*         info;
  pthread_mutex_t lock;
  textstyle_t     style;
} act = {0};

void ui_activity_popup_select(int index)
{
  printf("on_ui_activity_popupitem_select\n");
}

void ui_activity_popup_log(char* log)
{
  pthread_mutex_lock(&act.lock); // have to be thread safe

  char* last = act.logs->data[0];

  if (last && strstr(log, "progress"))
  {
    vec_replace_at_index(act.logs, log, 0);
  }
  else
    vec_ins(act.logs, log, 0);

  if (act.logs->length > 100) vec_rem_at_index(act.logs, 100);

  if (act.list)
  {
    textlist_update(act.list);

    tg_text_set(act.info, log, act.style);
  }

  pthread_mutex_unlock(&act.lock);
}

void ui_activity_popup_attach(view_t* baseview)
{
  act.logs = VNEW(); // REL 0

  view_t* listview = view_get_subview(baseview, "messages_popup_list");
  view_t* infoview = view_get_subview(baseview, "song_info");

  textstyle_t ts = {0};
  ts.font        = config_get("font_path");
  ts.size        = 30.0;
  ts.textcolor   = 0x000000FF;
  ts.backcolor   = 0x0;
  ts.align       = TA_LEFT;
  ts.margin      = 10.0;
  ts.multiline   = 1;

  act.style = ts;
  act.list  = textlist_new(listview, ts, ui_activity_popup_select); // REL 1
  act.info  = infoview;

  act.style.align = TA_CENTER;

  textlist_set_datasource(act.list, act.logs);
}

void ui_activity_popup_detach()
{
  REL(act.logs); // REL 0
  REL(act.list); // REL 1
}

#endif
