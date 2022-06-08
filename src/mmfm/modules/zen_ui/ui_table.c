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
    vec_t*   cache; // item cache
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

#include "config.c"
#include "tg_css.c"
#include "tg_text.c"
#include "vh_tbody.c"
#include "vh_tevnt.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_memory.c"
#include "zc_text.c"

void ui_table_del(
    void* p)
{
    ui_table_t* uit = p;
    REL(uit->id);
    REL(uit->cache);
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

    view_t* rowview = NULL;

    if (uit->items)
    {
	if (index > -1 && index < uit->items->length)
	{
	    map_t* data   = uit->items->data[index];
	    vec_t* fields = VNEW(); // REL 1
	    map_keys(data, fields);

	    textstyle_t ts = {0};
	    ts.font        = config_get("font_path");
	    ts.size        = 16.0;
	    ts.margin      = 5;
	    ts.align       = TA_LEFT;
	    ts.textcolor   = 0x000000FF;
	    ts.backcolor   = 0;
	    ts.multiline   = 0;

	    if (uit->cache->length > 0)
	    {
		rowview = uit->cache->data[0];
		VREM(uit->cache, 0);
	    }
	    else
	    {
		char* rowid = cstr_new_format(100, "%s_rowitem_%i", uit->id, uit->cnt++); // REL 0
		rowview     = view_new(rowid, (r2_t){0, 0, table_v->frame.local.w, 20});
		REL(rowid); // REL 0

		tg_css_add(rowview);

		for (int i = 0; i < fields->length; i++)
		{
		    char*   field    = fields->data[i];
		    char*   value    = MGET(data, field);
		    char*   cellid   = cstr_new_format(100, "%s_cell_%s", rowview->id, field); // REL 2
		    view_t* cellview = view_new(cellid, (r2_t){100 * i, 0, 100, 20});          // REL 3

		    tg_text_add(cellview);

		    REL(cellid);   // REL 2
		    REL(cellview); // REL 3

		    view_add_subview(rowview, cellview);
		}
	    }

	    if (index % 2 == 0)
	    {
		rowview->layout.background_color = 0xFF0000FF;
	    }
	    else
	    {
		rowview->layout.background_color = 0x440000FF;
	    }

	    for (int i = 0; i < fields->length; i++)
	    {
		char*   field    = fields->data[i];
		char*   value    = MGET(data, field);
		view_t* cellview = rowview->views->data[i];

		tg_text_set(cellview, value, ts);
	    }

	    view_set_frame(rowview, (r2_t){0, 0, fields->length * 100, 20});

	    REL(fields); // REL 1
	}
    }

    return rowview;
}

void ui_table_item_recycle(
    view_t* table_v,
    view_t* item_v,
    void*   userdata)
{
    ui_table_t* uit = (ui_table_t*) userdata;
    zc_log_debug("RECYCLE");
    VADD(uit->cache, item_v);
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
    uit->cache      = VNEW();
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
