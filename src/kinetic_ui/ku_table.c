#ifndef ku_table_h
#define ku_table_h

#include "ku_text.c"
#include "ku_view.c"
#include "mt_vector.c"
#include <stdint.h>

typedef struct _ku_table_t ku_table_t;

enum ku_table_event_id
{
    KU_TABLE_EVENT_SELECT,
    KU_TABLE_EVENT_OPEN,
    KU_TABLE_EVENT_CONTEXT,
    KU_TABLE_EVENT_DRAG,
    KU_TABLE_EVENT_DROP,
    KU_TABLE_EVENT_KEY_DOWN,
    KU_TABLE_EVENT_KEY_UP,
    KU_TABLE_EVENT_FIELDS_UPDATE,
    KU_TABLE_EVENT_FIELD_SELECT
};

typedef struct _ku_table_event_t
{
    enum ku_table_event_id id;
    ku_table_t*            table;
    char*                  field;
    mt_vector_t*           fields;
    mt_vector_t*           selected_items;
    int32_t                selected_index;
    ku_view_t*             rowview;
    ku_event_t             ev;
} ku_table_event_t;

struct _ku_table_t
{
    char*    id;  // unique id for item generation
    uint32_t cnt; // item count for item generation

    mt_vector_t* items;  // data items
    mt_vector_t* cache;  // item cache
    mt_vector_t* fields; // field name field size interleaved vector

    mt_vector_t* selected_items; // selected items
    int32_t      selected_index; // index of last selected
    ku_view_t*   body_v;
    ku_view_t*   evnt_v;
    ku_view_t*   scrl_v;

    textstyle_t rowastyle; // alternating row a style
    textstyle_t rowbstyle; // alternating row b style
    textstyle_t rowsstyle; // selected row textstyle
    textstyle_t headstyle; // header textstyle

    void (*on_event)(ku_table_event_t event);
};

ku_table_t* ku_table_create(
    char*        id,
    ku_view_t*   body,
    ku_view_t*   scrl,
    ku_view_t*   evnt,
    ku_view_t*   head,
    mt_vector_t* fields,
    textstyle_t  rowastyle,
    textstyle_t  rowbstyle,
    textstyle_t  rowsstyle,
    textstyle_t  headstyle,
    void (*on_event)(ku_table_event_t event));

void ku_table_select(
    ku_table_t* table,
    int32_t     index,
    int         add);

void ku_table_set_data(
    ku_table_t* uit, mt_vector_t* data);

mt_vector_t* ku_table_get_fields(ku_table_t* uit);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "ku_gen_textstyle.c"
#include "mt_log.c"
#include "mt_memory.c"
#include "mt_number.c"
#include "mt_string.c"
#include "tg_css.c"
#include "tg_text.c"
#include "vh_tbl_body.c"
#include "vh_tbl_evnt.c"
#include "vh_tbl_head.c"
#include "vh_tbl_scrl.c"
#include <xkbcommon/xkbcommon.h>

/* header order/size change, update cells */

void ku_table_head_update_cells(ku_table_t* uit, int fixed_index, int fixed_pos)
{
    for (int ri = 0; ri < uit->body_v->views->length; ri++)
    {
	ku_view_t* rowview = uit->body_v->views->data[ri];
	float      wth     = 0;

	for (int ci = 0; ci < rowview->views->length; ci++)
	{
	    ku_view_t*   cellview = rowview->views->data[ci];
	    ku_rect_t    frame    = cellview->frame.local;
	    mt_number_t* sizep    = uit->fields->data[ci * 2 + 1];
	    frame.x               = ci == fixed_index ? (float) fixed_pos : wth;
	    frame.w               = (float) sizep->intv;
	    ku_view_set_frame(cellview, frame);
	    wth += frame.w + 2;
	}

	ku_rect_t frame = rowview->frame.local;
	frame.w         = wth;
	ku_view_set_frame(rowview, frame);
    }
}

/* header field moved, update cells */

void ku_table_head_move(ku_view_t* hview, int index, int pos, void* userdata)
{
    ku_table_t* uit = (ku_table_t*) userdata;

    ku_table_head_update_cells(uit, index, pos);
}

/* header field resized, update cells */

void ku_table_head_resize(ku_view_t* hview, int index, int size, void* userdata)
{
    ku_table_t* uit = (ku_table_t*) userdata;

    if (index > -1)
    {
	mt_number_t* sizep = uit->fields->data[index * 2 + 1];
	sizep->intv        = size;

	ku_table_head_update_cells(uit, -1, 0);
    }
    else
    {
	ku_table_event_t event = {.table = uit, .id = KU_TABLE_EVENT_FIELDS_UPDATE, .fields = uit->fields};
	(*uit->on_event)(event);
    }
}

/* header field reordered, update cells */

void ku_table_head_reorder(ku_view_t* hview, int ind1, int ind2, void* userdata)
{
    ku_table_t* uit = (ku_table_t*) userdata;

    if (ind1 == -1)
    {
	/* self click, dispatch event */
	char*            field = uit->fields->data[ind2 * 2];
	ku_table_event_t event = {.id = KU_TABLE_EVENT_FIELD_SELECT, .field = field};
	(*uit->on_event)(event);
    }
    else
    {
	char*        field1 = uit->fields->data[ind1 * 2];
	mt_number_t* size1  = uit->fields->data[ind1 * 2 + 1];
	char*        field2 = uit->fields->data[ind2 * 2];
	mt_number_t* size2  = uit->fields->data[ind2 * 2 + 1];

	uit->fields->data[ind1 * 2]     = field2;
	uit->fields->data[ind1 * 2 + 1] = size2;

	uit->fields->data[ind2 * 2]     = field1;
	uit->fields->data[ind2 * 2 + 1] = size1;

	for (int ri = 0; ri < uit->body_v->views->length; ri++)
	{
	    ku_view_t* rowview = uit->body_v->views->data[ri];

	    ku_view_t* cell1 = RET(rowview->views->data[ind1]);
	    ku_view_t* cell2 = RET(rowview->views->data[ind2]);

	    ku_view_remove_subview(rowview, cell1);
	    ku_view_insert_subview(rowview, cell1, ind2);
	    ku_view_remove_subview(rowview, cell2);
	    ku_view_insert_subview(rowview, cell2, ind1);

	    REL(cell1);
	    REL(cell2);
	}

	ku_table_head_update_cells(uit, -1, 0);

	ku_table_event_t event = {.table = uit, .id = KU_TABLE_EVENT_FIELDS_UPDATE, .fields = uit->fields};
	(*uit->on_event)(event);
    }
}

/* ceeate header */

ku_view_t* ku_table_head_create(
    ku_view_t* head_v,
    void*      userdata)
{
    ku_table_t* uit = (ku_table_t*) userdata;

    char*      headid   = mt_string_new_format(100, "%s_header", uit->id); // REL 0
    ku_view_t* headview = ku_view_new(headid, (ku_rect_t){0, 0, 100, uit->headstyle.line_height});

    REL(headid);

    int wth = 0;

    /* create header fields/cells */

    for (int i = 0; i < uit->fields->length; i += 2)
    {
	char*        field    = uit->fields->data[i];
	mt_number_t* size     = uit->fields->data[i + 1];
	char*        cellid   = mt_string_new_format(100, "%s_cell_%s", headview->id, field);                     // REL 2
	ku_view_t*   cellview = ku_view_new(cellid, (ku_rect_t){wth, 0, size->intv, uit->headstyle.line_height}); // REL 3

	wth += size->intv + 2;

	tg_text_add(cellview);
	tg_text_set(cellview, field, uit->headstyle);

	ku_view_add_subview(headview, cellview);

	REL(cellid);
	REL(cellview);
    }

    ku_view_set_frame(headview, (ku_rect_t){0, 0, wth, uit->headstyle.line_height});

    return headview;
}

/* create item */

ku_view_t* ku_table_item_create(
    ku_view_t* table_v,
    int        index,
    void*      userdata)
{
    ku_table_t* uit = (ku_table_t*) userdata;

    ku_view_t* rowview = NULL;

    if (uit->items)
    {
	if (index > -1 && index < uit->items->length)
	{
	    mt_map_t* data = uit->items->data[index];

	    if (uit->cache->length > 0)
	    {
		/* get cached item */
		rowview = RET(uit->cache->data[0]);
		mt_vector_rem_at_index(uit->cache, 0);
	    }
	    else
	    {
		/* create new item */
		char* rowid = mt_string_new_format(100, "%s_rowitem_%i", uit->id, uit->cnt++);
		rowview     = ku_view_new(rowid, (ku_rect_t){0, 0, table_v->frame.local.w, uit->rowastyle.line_height});
		REL(rowid);

		/* create cells */
		int wth = 0;

		for (int i = 0; i < uit->fields->length; i += 2)
		{
		    char*        field    = uit->fields->data[i];
		    mt_number_t* size     = uit->fields->data[i + 1];
		    char*        cellid   = mt_string_new_format(100, "%s_cell_%s", rowview->id, field);                      // REL 2
		    ku_view_t*   cellview = ku_view_new(cellid, (ku_rect_t){wth, 0, size->intv, uit->rowastyle.line_height}); // REL 3

		    wth += size->intv + 2;

		    tg_text_add(cellview);

		    ku_view_add_subview(rowview, cellview);

		    REL(cellid);
		    REL(cellview);
		}
	    }

	    /* select style */
	    textstyle_t style = index % 2 == 0 ? uit->rowastyle : uit->rowbstyle;

	    if (uit->selected_items->length > 0)
	    {
		uint32_t pos = mt_vector_index_of_data(uit->selected_items, data);
		if (pos < UINT32_MAX) style = uit->rowsstyle;
	    }

	    /* update cells */
	    int wth = 0;

	    for (int i = 0; i < uit->fields->length; i += 2)
	    {
		char*        field    = uit->fields->data[i];
		mt_number_t* size     = uit->fields->data[i + 1];
		char*        value    = MGET(data, field);
		ku_view_t*   cellview = rowview->views->data[i / 2];
		ku_rect_t    frame    = cellview->frame.local;

		frame.x = wth;
		frame.w = size->intv;
		ku_view_set_frame(cellview, frame);

		wth += size->intv + 2;

		if (value) tg_text_set(cellview, value, style);
		else tg_text_set(cellview, "", style); // reset old value
	    }

	    ku_view_set_frame(rowview, (ku_rect_t){0, 0, wth, style.line_height});
	}
    }

    return rowview;
}

/* add item to cache */

void ku_table_item_recycle(
    ku_view_t* table_v,
    ku_view_t* item_v,
    void*      userdata)
{
    ku_table_t* uit = (ku_table_t*) userdata;
    VADD(uit->cache, item_v);
}

/* table event */

void ku_table_evnt_event(vh_tbl_evnt_event_t event)
{
    ku_table_t* uit = (ku_table_t*) event.userdata;

    if (event.id == VH_TBL_EVENT_SELECT)
    {
	uit->selected_index = event.index;

	mt_map_t* data = uit->items->data[event.index];

	uint32_t pos = mt_vector_index_of_data(uit->selected_items, data);

	if (pos == UINT32_MAX)
	{
	    /* reset selected if control is not down */

	    if (!event.ev.ctrl_down)
	    {
		mt_vector_reset(uit->selected_items);
		vh_tbl_body_t* bvh = uit->body_v->handler_data;

		for (int index = 0; index < bvh->items->length; index++)
		{
		    ku_view_t* item = bvh->items->data[index];

		    textstyle_t style = index % 2 == 0 ? uit->rowastyle : uit->rowbstyle;

		    for (int i = 0; i < item->views->length; i++)
		    {
			ku_view_t* cellview = item->views->data[i];
			tg_text_set_style(cellview, style);
		    }
		}
	    }

	    VADD(uit->selected_items, data);

	    for (int i = 0; i < event.rowview->views->length; i++)
	    {
		ku_view_t* cellview = event.rowview->views->data[i];
		tg_text_set_style(cellview, uit->rowsstyle);
	    }
	}
	else
	{
	    VREM(uit->selected_items, data);

	    textstyle_t style = event.index % 2 == 0 ? uit->rowastyle : uit->rowbstyle;

	    for (int i = 0; i < event.rowview->views->length; i++)
	    {
		ku_view_t* cellview = event.rowview->views->data[i];
		tg_text_set_style(cellview, style);
	    }
	}

	ku_table_event_t tevent = {
	    .table          = uit,
	    .id             = KU_TABLE_EVENT_SELECT,
	    .selected_items = uit->selected_items,
	    .selected_index = event.index,
	    .rowview        = event.rowview};

	(*uit->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_CONTEXT)
    {
	uit->selected_index = event.index;

	mt_map_t* data = uit->items->data[event.index];

	uint32_t pos = mt_vector_index_of_data(uit->selected_items, data);

	if (pos == UINT32_MAX)
	{
	    /* reset selected if control is not down */
	    if (!event.ev.ctrl_down)
	    {
		mt_vector_reset(uit->selected_items);
		vh_tbl_body_t* bvh = uit->body_v->handler_data;

		for (int index = 0; index < bvh->items->length; index++)
		{
		    ku_view_t* item = bvh->items->data[index];
		    if (item->style.background_color == 0x006600FF)
		    {
			item->style.background_color = 0x000000FF;
			ku_view_invalidate_texture(item);
		    }
		}
	    }

	    VADD(uit->selected_items, data);

	    if (event.rowview)
	    {
		event.rowview->style.background_color = 0x006600FF;
		ku_view_invalidate_texture(event.rowview);
	    }
	}

	ku_table_event_t tevent = {
	    .table          = uit,
	    .id             = KU_TABLE_EVENT_CONTEXT,
	    .selected_items = uit->selected_items,
	    .selected_index = event.index,
	    .rowview        = event.rowview,
	    .ev             = event.ev};
	(*uit->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_OPEN)
    {
	ku_table_event_t tevent = {
	    .table          = uit,
	    .id             = KU_TABLE_EVENT_OPEN,
	    .selected_items = uit->selected_items,
	    .selected_index = event.index,
	    .rowview        = event.rowview};
	(*uit->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_DRAG)
    {
	ku_table_event_t tevent = {
	    .table          = uit,
	    .id             = KU_TABLE_EVENT_DRAG,
	    .selected_items = uit->selected_items,
	    .selected_index = event.index,
	    .rowview        = event.rowview};
	(*uit->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_DROP)
    {
	ku_table_event_t tevent = {
	    .table          = uit,
	    .id             = KU_TABLE_EVENT_DROP,
	    .selected_items = uit->selected_items,
	    .selected_index = event.index,
	    .rowview        = event.rowview};
	(*uit->on_event)(tevent);
    }
    else if (event.id == VH_TBL_EVENT_KEY_DOWN)
    {
	ku_table_event_t tevent = {
	    .table          = uit,
	    .id             = KU_TABLE_EVENT_KEY_DOWN,
	    .selected_items = uit->selected_items,
	    .selected_index = uit->selected_index,
	    .rowview        = event.rowview,
	    .ev             = event.ev};
	(*uit->on_event)(tevent);

	if (event.ev.keycode == XKB_KEY_Down || event.ev.keycode == XKB_KEY_Up)
	{
	    if (event.ev.keycode == XKB_KEY_Down) ku_table_select(uit, uit->selected_index + 1, event.ev.shift_down);
	    if (event.ev.keycode == XKB_KEY_Up) ku_table_select(uit, uit->selected_index - 1, event.ev.shift_down);

	    tevent = (ku_table_event_t){
		.table          = uit,
		.id             = KU_TABLE_EVENT_SELECT,
		.selected_items = uit->selected_items,
		.selected_index = uit->selected_index,
		.rowview        = vh_tbl_body_item_for_index(uit->body_v, uit->selected_index)};

	    (*uit->on_event)(tevent);
	}

	if (event.ev.keycode == XKB_KEY_Return)
	{
	    tevent = (ku_table_event_t){
		.table          = uit,
		.id             = KU_TABLE_EVENT_OPEN,
		.selected_items = uit->selected_items,
		.selected_index = event.index,
		.rowview        = event.rowview};
	    (*uit->on_event)(tevent);
	}
    }
    else if (event.id == VH_TBL_EVENT_KEY_UP)
    {
	ku_table_event_t tevent = (ku_table_event_t){
	    .table          = uit,
	    .id             = KU_TABLE_EVENT_KEY_UP,
	    .selected_items = uit->selected_items,
	    .selected_index = uit->selected_index,
	    .rowview        = event.rowview,
	    .ev             = event.ev};
	(*uit->on_event)(tevent);
    }
}

void ku_table_del(
    void* p)
{
    ku_table_t* uit = p;

    REL(uit->id);             // REL S0
    REL(uit->cache);          // REL S1
    REL(uit->fields);         // REL S2
    REL(uit->selected_items); // REL S3

    if (uit->items) REL(uit->items);

    if (uit->body_v) REL(uit->body_v);
    if (uit->evnt_v) REL(uit->evnt_v);
    if (uit->scrl_v) REL(uit->scrl_v);
}

void ku_table_desc(
    void* p,
    int   level)
{
    printf("ku_table");
}

ku_table_t* ku_table_create(
    char*        id,
    ku_view_t*   body,
    ku_view_t*   scrl,
    ku_view_t*   evnt,
    ku_view_t*   head,
    mt_vector_t* fields,
    textstyle_t  rowastyle,
    textstyle_t  rowbstyle,
    textstyle_t  rowsstyle,
    textstyle_t  headstyle,
    void (*on_event)(ku_table_event_t event))
{
    assert(id != NULL);
    assert(body != NULL);

    ku_table_t* uit     = CAL(sizeof(ku_table_t), ku_table_del, ku_table_desc);
    uit->id             = mt_string_new_cstring(id);
    uit->cache          = VNEW();
    uit->fields         = RET(fields);
    uit->selected_items = VNEW();
    uit->on_event       = on_event;

    uit->rowastyle = rowastyle;
    uit->rowbstyle = rowbstyle;
    uit->rowsstyle = rowsstyle;
    uit->headstyle = headstyle;

    uit->body_v = RET(body);

    vh_tbl_body_attach(
	body,
	ku_table_item_create,
	ku_table_item_recycle,
	uit);

    if (head)
    {
	vh_tbl_head_attach(
	    head,
	    ku_table_head_create,
	    ku_table_head_move,
	    ku_table_head_resize,
	    ku_table_head_reorder,
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
	    ku_table_evnt_event,
	    uit);
    }

    return uit;
}

/* data items have to be maps containing the same keys */

void ku_table_set_data(
    ku_table_t*  uit,
    mt_vector_t* data)
{
    if (uit->items) REL(uit->items);
    uit->items = RET(data);

    uit->selected_index = 0;

    mt_vector_reset(uit->selected_items);
    if (uit->selected_index < uit->items->length)
    {
	mt_map_t* sel = uit->items->data[uit->selected_index];
	VADD(uit->selected_items, sel);
    }

    vh_tbl_body_reset(uit->body_v);
    vh_tbl_body_move(uit->body_v, 0, 0);

    if (uit->scrl_v) vh_tbl_scrl_set_item_count(uit->scrl_v, data->length);
}

/* select index */

void ku_table_select(
    ku_table_t* uit,
    int32_t     index,
    int         add)
{
    vh_tbl_body_t* bvh = uit->body_v->handler_data;

    uit->selected_index = index;
    if (uit->selected_index < 0) uit->selected_index = 0;
    if (uit->selected_index > uit->items->length - 1) uit->selected_index = uit->items->length - 1;

    if (bvh->bot_index < uit->selected_index)
    {
	vh_tbl_body_vjump(uit->body_v, uit->selected_index + 1, 0);

	if (bvh->tail_index == bvh->bot_index)
	{
	    /* check if bottom item is out of bounds */
	    ku_view_t* lastitem = mt_vector_tail(bvh->items);
	    ku_rect_t  iframe   = lastitem->frame.local;
	    ku_rect_t  vframe   = uit->body_v->frame.local;

	    if (iframe.y + iframe.h > vframe.h)
	    {
		vh_tbl_body_move(uit->body_v, 0, iframe.y + iframe.h - vframe.h - 20.0);
	    }
	}
    }

    if (uit->selected_index <= bvh->top_index)
    {
	vh_tbl_body_vjump(uit->body_v, uit->selected_index - 1, 1);
    }

    if (add == 0) mt_vector_reset(uit->selected_items);
    mt_map_t* sel = uit->items->data[uit->selected_index];
    VADD(uit->selected_items, sel);

    /* color item */

    for (int i = 0; i < bvh->items->length; i++)
    {
	int        realindex = bvh->head_index + i;
	mt_map_t*  data      = uit->items->data[realindex];
	ku_view_t* item      = bvh->items->data[i];

	textstyle_t style = realindex % 2 == 0 ? uit->rowastyle : uit->rowbstyle;
	if (mt_vector_index_of_data(uit->selected_items, data) < UINT32_MAX) style = uit->rowsstyle;

	for (int i = 0; i < item->views->length; i++)
	{
	    ku_view_t* cellview = item->views->data[i];
	    tg_text_set_style(cellview, style);
	}
    }

    if (uit->scrl_v) vh_tbl_scrl_update(uit->scrl_v);
}

mt_vector_t* ku_table_get_fields(ku_table_t* uit)
{
    return uit->fields;
}

#endif
