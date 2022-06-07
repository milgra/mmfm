#ifndef ui_table_h
#define ui_table_h

#include "view.c"
#include "zc_vector.c"
#include <stdint.h>

typedef struct _ui_table_t
{
    char*    id;    // unique id for item generation
    uint32_t cnt;   // item count for item generation
    vec_t*   items; // data items
    view_t*  body_v;
    view_t*  evnt_v;
    view_t*  scrl_v;
} ui_table_t;

ui_table_t* ui_table_create(
    char*   id,
    view_t* body,
    view_t* evnt,
    view_t* scrl);

void ui_table_set_data(
    ui_table_t* uit,
    vec_t*      data);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "tg_css.c"
#include "vh_tbody.c"
#include "vh_tevnt.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_memory.c"

void ui_table_del(
    void* p)
{
    ui_table_t* uit = p;
    REL(uit->id);
    if (uit->body_v) REL(uit->body_v);
    if (uit->evnt_v) REL(uit->evnt_v);
    if (uit->scrl_v) REL(uit->scrl_v);
}

void ui_table_desc(
    void* p,
    int   level)
{
    printf("vh_tbody");
}

view_t* ui_table_item_create(
    view_t* table_v,
    int     index,
    void*   userdata)
{
    ui_table_t* uit = (ui_table_t*) userdata;

    zc_log_debug("ui table item create %s : %i", table_v->id, index);

    view_t* rowview = NULL;

    if (uit->items)
    {
	if (index > -1 && index < uit->items->length)
	{
	    char idbuffer[100] = {0};
	    snprintf(idbuffer, 100, "%s%i", uit->id, uit->cnt++);

	    rowview = view_new(idbuffer, (r2_t){0, 0, 200, 20}); // REL 0

	    rowview->layout.background_color = 0xFF0000FF;

	    tg_css_add(rowview);

	    zc_log_debug("ui table item create %s", idbuffer);
	}
    }

    return rowview;
}

void ui_table_item_recycle(
    view_t* table_v,
    view_t* item_v,
    void*   userdata)
{
}

/* id has to be uniqe */

ui_table_t* ui_table_create(
    char*   id,
    view_t* body,
    view_t* evnt,
    view_t* scrl)
{
    assert(id != NULL);

    ui_table_t* uit = CAL(sizeof(ui_table_t), ui_table_del, ui_table_desc);
    uit->id         = cstr_new_cstring(id);
    if (body) uit->body_v = RET(body);
    if (evnt) uit->evnt_v = RET(evnt);
    if (scrl) uit->scrl_v = RET(scrl);

    vh_tbody_attach(
	body,
	ui_table_item_create,
	ui_table_item_recycle,
	uit);

    vh_tevnt_attach(
	evnt,
	body,
	uit);

    zc_log_debug("ui table create %s", id);

    return uit;
}

/* data items have to be maps containing the same keys */

void ui_table_set_data(
    ui_table_t* uit,
    vec_t*      data)
{
    uit->items = RET(data);

    zc_log_debug("ui table set data %i", data->length);

    vh_tbody_move(uit->body_v, 0, 0);
}

#endif
