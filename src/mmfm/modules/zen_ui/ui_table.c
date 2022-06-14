#ifndef ui_table_h
#define ui_table_h

#include "view.c"
#include "zc_text.c"
#include "zc_vector.c"
#include <stdint.h>

typedef struct _ui_table_t
{
    char*       id;    // unique id for item generation
    uint32_t    cnt;   // item count for item generation
    vec_t*      items; // data items
    vec_t*      cache; // item cache
    vec_t*      fields;
    view_t*     body_v;
    view_t*     evnt_v;
    view_t*     scrl_v;
    textstyle_t textstyle;
    void (*fields_update)(char* id, vec_t* fields);
} ui_table_t;

ui_table_t* ui_table_create(
    char*   id,
    view_t* body,
    view_t* scrl,
    view_t* evnt,
    view_t* head,
    vec_t*  fields,
    void (*fields_update)(char* id, vec_t* fields));

void ui_table_set_data(
    ui_table_t* uit, vec_t* data);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_util.c"
#include "vh_tbl_body.c"
#include "vh_tbl_evnt.c"
#include "vh_tbl_head.c"
#include "vh_tbl_scrl.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_memory.c"
#include "zc_number.c"

void ui_table_del(
    void* p)
{
    ui_table_t* uit = p;
    REL(uit->id);
    REL(uit->cache);
    REL(uit->fields);
    if (uit->body_v) REL(uit->body_v);
    if (uit->evnt_v) REL(uit->evnt_v);
    if (uit->scrl_v) REL(uit->scrl_v);
}

void ui_table_desc(
    void* p,
    int   level)
{
    printf("ui_table");
}

void ui_table_head_resize(view_t* hview, int index, int size)
{
    zc_log_debug("head resize %i %i", index, size);
}
void ui_table_head_reorder(view_t* hview, int ind1, int ind2)
{
    zc_log_debug("head reorder %i %i", ind1, ind2);
}

view_t* ui_table_head_create(
    view_t* head_v,
    void*   userdata)
{
    ui_table_t* uit = (ui_table_t*) userdata;

    char*   headid   = cstr_new_format(100, "%s_header", uit->id); // REL 0
    view_t* headview = view_new(headid, (r2_t){0, 0, 100, 20});
    REL(headid); // REL 0

    int         wth = 0;
    textstyle_t ts  = uit->textstyle;
    ts.backcolor    = 0xFF4455FF;

    for (int i = 0; i < uit->fields->length; i += 2)
    {
	char*   field    = uit->fields->data[i];
	num_t*  size     = uit->fields->data[i + 1];
	char*   cellid   = cstr_new_format(100, "%s_cell_%s", headview->id, field);  // REL 2
	view_t* cellview = view_new(cellid, (r2_t){wth + 1, 0, size->intv - 2, 20}); // REL 3

	wth += size->intv;

	tg_text_add(cellview);
	tg_text_set(cellview, field, ts);

	REL(cellid);   // REL 2
	REL(cellview); // REL 3

	view_add_subview(headview, cellview);
    }

    view_set_frame(headview, (r2_t){0, 0, wth, 20});

    zc_log_debug("CREATE");

    return headview;
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
	    map_t* data = uit->items->data[index];

	    textstyle_t ts = uit->textstyle;
	    if (index % 2 != 0) ts.backcolor |= 0xEDEDEDCC;

	    if (uit->cache->length > 0)
	    {
		rowview = uit->cache->data[0];
		vec_rem_at_index(uit->cache, 0);
	    }
	    else
	    {
		char* rowid = cstr_new_format(100, "%s_rowitem_%i", uit->id, uit->cnt++); // REL 0
		rowview     = view_new(rowid, (r2_t){0, 0, table_v->frame.local.w, 20});
		REL(rowid); // REL 0

		int wth = 0;

		for (int i = 0; i < uit->fields->length; i += 2)
		{
		    char*   field    = uit->fields->data[i];
		    num_t*  size     = uit->fields->data[i + 1];
		    char*   value    = MGET(data, field);
		    char*   cellid   = cstr_new_format(100, "%s_cell_%s", rowview->id, field);   // REL 2
		    view_t* cellview = view_new(cellid, (r2_t){wth + 1, 0, size->intv - 2, 20}); // REL 3

		    wth += size->intv;

		    tg_text_add(cellview);

		    REL(cellid);   // REL 2
		    REL(cellview); // REL 3

		    view_add_subview(rowview, cellview);
		}
	    }

	    int wth = 0;

	    for (int i = 0; i < uit->fields->length; i += 2)
	    {
		char*   field    = uit->fields->data[i];
		num_t*  size     = uit->fields->data[i + 1];
		char*   value    = MGET(data, field);
		view_t* cellview = rowview->views->data[i / 2];

		wth += size->intv;

		tg_text_set(cellview, value, ts);
	    }

	    view_set_frame(rowview, (r2_t){0, 0, wth, 20});
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
    VADD(uit->cache, item_v);
}

/* id has to be uniqe */

ui_table_t* ui_table_create(
    char*   id,
    view_t* body,
    view_t* scrl,
    view_t* evnt,
    view_t* head,
    vec_t*  fields,
    void (*fields_update)(char* id, vec_t* fields))
{
    assert(id != NULL);
    assert(body != NULL);

    ui_table_t* uit    = CAL(sizeof(ui_table_t), ui_table_del, ui_table_desc);
    uit->id            = cstr_new_cstring(id);
    uit->cache         = VNEW();
    uit->fields        = RET(fields);
    uit->fields_update = fields_update;

    uit->body_v = RET(body);

    vh_tbl_body_attach(
	body,
	ui_table_item_create,
	ui_table_item_recycle,
	uit);

    uit->textstyle             = ui_util_gen_textstyle(body);
    uit->textstyle.margin_left = 5;

    if (head)
    {
	vh_tbl_head_attach(
	    head,
	    ui_table_head_create,
	    ui_table_head_resize,
	    ui_table_head_reorder,
	    uit);
    }

    if (scrl)
    {
	uit->scrl_v = RET(scrl);

	vh_tbl_scrl_attach(
	    scrl,
	    body,
	    head,
	    uit);
    }

    if (evnt)
    {
	uit->evnt_v = RET(evnt);
	vh_tbl_evnt_attach(
	    evnt,
	    body,
	    scrl,
	    head,
	    uit);
    }

    return uit;
}

/* data items have to be maps containing the same keys */

void ui_table_set_data(
    ui_table_t* uit,
    vec_t*      data)
{
    uit->items = RET(data);

    zc_log_debug("ui table set data %i", data->length);

    vh_tbl_body_move(uit->body_v, 0, 0);

    if (uit->scrl_v) vh_tbl_scrl_set_item_count(uit->scrl_v, data->length);
}

#endif
