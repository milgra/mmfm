#ifndef ku_view_h
#define ku_view_h

#include "ku_bitmap.c"
#include "ku_event.c"
#include "ku_rect.c"
#include "zc_vector.c"
#include <math.h>

typedef enum _laypos_t // layout position
{
    LP_STATIC = 0,
    LP_FIXED,
    LP_ABSOLUTE,
} laypos_t;

typedef enum _laydis_t // layout display
{
    LD_NONE = 0,
    LD_FLEX,
} laydis_t;

typedef enum _flexdir_t // flexdir
{
    FD_ROW = 0,
    FD_COL,
} flexdir_t;

typedef enum _itemalign_t // flexdir
{
    IA_NONE = 0,
    IA_CENTER,
} itemalign_t;

typedef enum _cjustify_t // justify content
{
    JC_NONE = 0,
    JC_CENTER,
} cjustify_t;

typedef struct _vstyle_t vstyle_t; // view style
struct _vstyle_t
{
    /* css dimension */

    int width;
    int height;

    float w_per;
    float h_per;

    /* css position */

    int top;
    int left;
    int right;
    int bottom;

    int margin;
    int margin_top;
    int margin_left;
    int margin_right;
    int margin_bottom;

    /* css decoration */

    int border_radius;

    int shadow_h;
    int shadow_w;
    int shadow_blur;
    int shadow_color;

    uint32_t background_color;
    char*    background_image;

    /* css layout */

    laypos_t    position;
    laydis_t    display;
    flexdir_t   flexdir;
    itemalign_t itemalign;
    cjustify_t  cjustify;

    /* css text */

    char*    font_family;
    float    font_size;
    uint32_t color;
    int      text_align;
    float    line_height;
    int      word_wrap;

    /* non-css */

    char masked; /* view should be used as mask for subviews? OVERFLOW */
    char unmask; /* masking should be stopped, helper variable */
};

typedef enum _texst_t // texture loading state
{
    TS_BLANK,   /* texture is empty */
    TS_PENDING, /* texture is under generation */
    TS_READY,   /* texture is generated */
    TS_STORED,  /* texture is stored in texmap */
} texst_t;

typedef enum _textype_t
{
    TT_MANAGED,
    TT_EXTERNAL
} textype_t;

typedef struct _texture_t
{
    textype_t type; /* managed or external */
    uint32_t  page; /* texture page */
    float     alpha;
    char      resizable; /* if view is resized initiate texture re-rendering */

    char transparent;
    // internal texture

    texst_t      state;         /* render state of texture */
    ku_bitmap_t* bitmap;        /* texture bitmap */
    char         changed;       /* texture is changed */
    char         uploaded;      /* texture is uploaded, used by renderes */
    char         alpha_changed; /* alpha channel is changed */

    // decoration

    char full;
    char blur;
} texture_t;

typedef struct _frame_t
{
    ku_rect_t local;  // local position
    ku_rect_t global; // global position
    ku_rect_t region; // region to show
    char      pos_changed;
    char      dim_changed;
    char      reg_changed;
} frame_t;

typedef struct _ku_view_t ku_view_t;
struct _ku_view_t
{
    char rearrange; /* subview structure changed, window needs rearrange */

    char needs_key;   /* accepts key events */
    char needs_text;  /* accepts text events */
    char needs_time;  /* accepts time events */
    char needs_touch; /* accepts touch events */

    char blocks_key;    /* blocks key events */
    char blocks_touch;  /* blocks touch events */
    char blocks_scroll; /* blocks scroll events */

    char* id;          /* identifier for handling view */
    char* class;       /* css class(es) */
    char*      script; /* script */
    char*      type;   /* html type (button,checkbox) */
    char*      text;   /* html text */
    vec_t*     views;  /* subviews */
    ku_view_t* parent; /* parent view */
    uint32_t   index;  /* depth */

    frame_t   frame;
    vstyle_t  style;
    texture_t texture;

    void (*handler)(ku_view_t*, ku_event_t); /* view handler for view */
    void (*tex_gen)(ku_view_t*);             /* texture generator for view */
    void* handler_data;                      /* data for event handler */
    void* tex_gen_data;                      /* data for texture generator */
};

ku_view_t* ku_view_new(char* id, ku_rect_t frame);
void       ku_view_set_type(ku_view_t* view, char* type);
void       ku_view_set_text(ku_view_t* view, char* type);
void       ku_view_set_class(ku_view_t* view, char* class);
void       ku_view_set_script(ku_view_t* view, char* script);
void       ku_view_add_subview(ku_view_t* view, ku_view_t* subview);
void       ku_view_remove_subview(ku_view_t* view, ku_view_t* subview);
void       ku_view_insert_subview(ku_view_t* view, ku_view_t* subview, uint32_t index);
void       ku_view_remove_from_parent(ku_view_t* view);
void       ku_view_set_parent(ku_view_t* view, ku_view_t* parent);

void       ku_view_evt(ku_view_t* view, ku_event_t ev); /* general event, sending to all views */
void       ku_view_coll_touched(ku_view_t* view, ku_event_t ev, vec_t* queue);
ku_view_t* ku_view_get_subview(ku_view_t* view, char* id);
void       ku_view_gen_texture(ku_view_t* view);
void       ku_view_set_masked(ku_view_t* view, char masked);
void       ku_view_set_frame(ku_view_t* view, ku_rect_t frame);
void       ku_view_set_region(ku_view_t* view, ku_rect_t frame);
void       ku_view_set_style(ku_view_t* view, vstyle_t style);
void       ku_view_set_block_touch(ku_view_t* view, char block, char recursive);
void       ku_view_set_texture_bmp(ku_view_t* view, ku_bitmap_t* tex);
void       ku_view_set_texture_page(ku_view_t* view, uint32_t page);
void       ku_view_set_texture_type(ku_view_t* view, textype_t type);
void       ku_view_set_texture_alpha(ku_view_t* view, float alpha, char recur);
void       ku_view_invalidate_texture(ku_view_t* view);
void       ku_view_layout(ku_view_t* view);

void ku_view_describe(void* pointer, int level);
void ku_view_desc(void* pointer, int level);
void ku_view_desc_style(vstyle_t l);
void ku_view_calc_global(ku_view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_memory.c"
#include <limits.h>

int ku_view_cnt = 0;

void ku_view_del(void* pointer)
{
    ku_view_t* view = (ku_view_t*) pointer;

    if (view->style.background_image != NULL) REL(view->style.background_image);
    if (view->style.font_family != NULL) REL(view->style.font_family);

    if (view->handler_data) REL(view->handler_data);
    if (view->tex_gen_data) REL(view->tex_gen_data);

    if (view->texture.bitmap) REL(view->texture.bitmap); // not all views has texture
    if (view->class) REL(view->class);
    if (view->type) REL(view->type);
    if (view->text) REL(view->text);
    if (view->script) REL(view->script);

    REL(view->id);
    REL(view->views);
}

ku_view_t* ku_view_new(char* id, ku_rect_t frame)
{
    ku_view_t* view         = CAL(sizeof(ku_view_t), ku_view_del, ku_view_desc);
    view->id                = cstr_new_cstring(id);
    view->views             = VNEW();
    view->frame.local       = frame;
    view->frame.global      = frame;
    view->texture.page      = -1;
    view->texture.alpha     = 1.0;
    view->texture.resizable = 1;
    view->needs_touch       = 1;
    // view->blocks_touch      = 1;

    // reset margins

    view->style.margin_top    = INT_MAX;
    view->style.margin_left   = INT_MAX;
    view->style.margin_right  = INT_MAX;
    view->style.margin_bottom = INT_MAX;
    view->style.top           = INT_MAX;
    view->style.left          = INT_MAX;
    view->style.right         = INT_MAX;
    view->style.bottom        = INT_MAX;
    view->style.shadow_color  = 0x00000033;

    // starts with changed states

    view->frame.dim_changed = 1;
    view->frame.pos_changed = 1;

    return view;
}

void ku_view_set_type(ku_view_t* view, char* type)
{
    if (view->type) REL(view->type);
    if (type) view->type = RET(type);
}

void ku_view_set_text(ku_view_t* view, char* text)
{
    if (view->text) REL(view->text);
    if (text) view->text = RET(text);
}

void ku_view_set_class(ku_view_t* view, char* class)
{
    if (view->class) REL(view->class);
    if (class) view->class = RET(class);
}

void ku_view_set_script(ku_view_t* view, char* script)
{
    if (view->script) REL(view->script);
    if (script) view->script = RET(script);
}

void ku_view_set_masked(ku_view_t* view, char masked)
{
    view->style.masked = 1;
}

void ku_view_add_subview(ku_view_t* view, ku_view_t* subview)
{
    for (int i = 0; i < view->views->length; i++)
    {
	ku_view_t* sview = view->views->data[i];
	if (strcmp(sview->id, subview->id) == 0)
	{
	    printf("DUPLICATE SUBVIEW %s\n", subview->id);
	    return;
	}
    }

    /* notify root about rearrange */
    ku_view_t* tview = view;
    while (tview->parent != NULL) tview = tview->parent;
    tview->rearrange = 1;

    ku_view_set_parent(subview, view);

    VADD(view->views, subview);

    ku_view_calc_global(view);
}

void ku_view_insert_subview(ku_view_t* view, ku_view_t* subview, uint32_t index)
{
    for (int i = 0; i < view->views->length; i++)
    {
	ku_view_t* sview = view->views->data[i];
	if (strcmp(sview->id, subview->id) == 0)
	{
	    printf("DUPLICATE SUBVIEW %s\n", subview->id);
	    return;
	}
    }

    vec_ins(view->views, subview, index);

    /* notify root about rearrange */
    ku_view_t* tview = view;
    while (tview->parent != NULL) tview = tview->parent;
    tview->rearrange = 1;

    ku_view_set_parent(subview, view);

    ku_view_calc_global(view);
}

void ku_view_remove_subview(ku_view_t* view, ku_view_t* subview)
{
    char success = VREM(view->views, subview);

    if (success == 1)
    {
	/* notify root about rearrange */
	ku_view_t* tview = view;
	while (tview->parent != NULL) tview = tview->parent;
	tview->rearrange = 1;

	ku_view_set_parent(subview, NULL);
    }
}

void ku_view_remove_from_parent(ku_view_t* view)
{
    if (view->parent) ku_view_remove_subview(view->parent, view);
}

void ku_view_set_parent(ku_view_t* view, ku_view_t* parent)
{
    view->parent = parent;
}

void ku_view_coll_touched(ku_view_t* view, ku_event_t ev, vec_t* queue)
{
    if (ev.x <= view->frame.global.x + view->frame.global.w &&
	ev.x >= view->frame.global.x &&
	ev.y <= view->frame.global.y + view->frame.global.h &&
	ev.y >= view->frame.global.y)
    {
	vec_add_unique_data(queue, view);
	for (int i = 0; i < view->views->length; i++)
	{
	    ku_view_t* v = view->views->data[i];
	    ku_view_coll_touched(v, ev, queue);
	}
    }
}

ku_view_t* ku_view_get_subview(ku_view_t* view, char* id)
{
    if (strcmp(view->id, id) == 0) return view;
    for (int i = 0; i < view->views->length; i++)
    {
	ku_view_t* sv = view->views->data[i];
	ku_view_t* re = ku_view_get_subview(sv, id);
	if (re) return re;
    }
    return NULL;
}

void ku_view_evt(ku_view_t* view, ku_event_t ev)
{
    for (int i = 0; i < view->views->length; i++)
    {
	ku_view_t* v = view->views->data[i];
	ku_view_evt(v, ev);
    }

    if (view->handler) (*view->handler)(view, ev);
}

void ku_view_calc_global(ku_view_t* view)
{
    ku_rect_t frame_parent = {0};
    if (view->parent != NULL) frame_parent = view->parent->frame.global;

    ku_rect_t frame_global = view->frame.local;
    ku_rect_t old_global   = view->frame.global;

    frame_global.x = frame_parent.x + frame_global.x;
    frame_global.y = frame_parent.y + frame_global.y;

    // notify about pos change

    if (fabs(frame_global.x - old_global.x) > 0.001 ||
	fabs(frame_global.y - old_global.y) > 0.001) view->frame.pos_changed = 1;

    view->frame.global = frame_global;

    for (int i = 0; i < view->views->length; i++)
    {
	ku_view_t* v = view->views->data[i];
	ku_view_calc_global(v);
    }
}

void ku_view_set_frame(ku_view_t* view, ku_rect_t frame)
{
    // check if texture needs rerendering

    if (view->frame.local.w != frame.w ||
	view->frame.local.h != frame.h)
    {
	view->frame.dim_changed = 1;
	if (frame.w >= 1.0 && frame.h >= 1.0)
	{
	    if (view->texture.type == TT_MANAGED && view->texture.resizable == 1)
	    {
		view->texture.state = TS_BLANK;
	    }
	}
    }

    view->frame.local = frame;

    ku_view_calc_global(view);
}

void ku_view_set_region(ku_view_t* view, ku_rect_t region)
{
    view->frame.region      = region;
    view->frame.reg_changed = 1;
}

void ku_view_set_block_touch(ku_view_t* view, char block, char recursive)
{
    view->blocks_touch = block;

    if (recursive)
    {
	for (int i = 0; i < view->views->length; i++)
	{
	    ku_view_t* v = view->views->data[i];
	    ku_view_set_block_touch(v, block, recursive);
	}
    }
}

void ku_view_set_texture_bmp(ku_view_t* view, ku_bitmap_t* bitmap)
{
    if (view->texture.bitmap) REL(view->texture.bitmap);
    view->texture.bitmap  = RET(bitmap);
    view->texture.state   = TS_READY;
    view->texture.changed = 1;
}

void ku_view_set_texture_page(ku_view_t* view, uint32_t page)
{
    view->texture.page = page;
}

void ku_view_set_texture_type(ku_view_t* view, textype_t type)
{
    view->texture.type = type;
}

void ku_view_set_texture_alpha(ku_view_t* view, float alpha, char recur)
{
    view->texture.alpha         = alpha;
    view->texture.alpha_changed = 1;

    if (recur)
    {
	for (int i = 0; i < view->views->length; i++)
	{
	    ku_view_t* v = view->views->data[i];
	    ku_view_set_texture_alpha(v, alpha, recur);
	}
    }
}

void ku_view_invalidate_texture(ku_view_t* view)
{
    view->texture.state = TS_BLANK;
}

void ku_view_set_style(ku_view_t* view, vstyle_t style)
{
    view->style = style;
}

void ku_view_gen_texture(ku_view_t* view)
{
    if (view->tex_gen) (*view->tex_gen)(view);
}

void ku_view_layout(ku_view_t* view)
{
    float act_x = 0;
    float act_y = 0;
    float rel_w = view->frame.local.w; // remaining width for relative views
    float rel_h = view->frame.local.h; // remaining height for relative views
    int   rem_w = view->views->length; // remaining relative views for width
    int   rem_h = view->views->length; // remaining relative views for height

    if (view->style.display == LD_FLEX)
    {
	if (view->style.flexdir == FD_ROW)
	{
	    // calculate width of all fixed width views
	    for (int i = 0; i < view->views->length; i++)
	    {
		ku_view_t* v = view->views->data[i];
		if (v->style.width > 0)
		{
		    rel_w -= v->style.width;
		    rem_w -= 1;
		}
		v->frame.local.x = 0;
		v->frame.local.y = 0;
	    }
	}
	if (view->style.flexdir == FD_COL)
	{
	    // calculate height of all fixed height views
	    for (int i = 0; i < view->views->length; i++)
	    {
		ku_view_t* v = view->views->data[i];
		if (v->style.height > 0)
		{
		    rel_h -= v->style.height;
		    rem_h -= 1;
		}
		v->frame.local.x = 0;
		v->frame.local.y = 0;
	    }
	}
    }

    for (int i = 0; i < view->views->length; i++)
    {
	ku_view_t* v     = view->views->data[i];
	ku_rect_t  frame = v->frame.local;

	if (v->style.margin == INT_MAX)
	{
	    frame.x = 0;
	    frame.y = 0;
	}

	if (v->style.margin_top < INT_MAX || v->style.margin_bottom < INT_MAX)
	{
	    frame.y = 0;
	}

	if (v->style.margin_left < INT_MAX || v->style.margin_right < INT_MAX)
	{
	    frame.x = 0;
	}

	if (v->style.width > 0)
	{
	    frame.w = v->style.width;
	    if (view->style.display == LD_FLEX && view->style.flexdir == FD_ROW)
	    {
		frame.x = act_x;
		act_x += frame.w;
	    }
	}
	if (v->style.height > 0)
	{
	    frame.h = v->style.height;
	    if (view->style.display == LD_FLEX && view->style.flexdir == FD_COL)
	    {
		frame.y = act_y;
		act_y += frame.h;
	    }
	}
	if (v->style.w_per > 0.0)
	{
	    float width = rel_w;
	    if (view->style.display == LD_FLEX && view->style.flexdir == FD_ROW)
	    {
		width   = floorf(rel_w / rem_w);
		frame.x = act_x;
		act_x += width;
		rem_w -= 1;
		rel_w -= width;
	    }
	    frame.w = width * v->style.w_per;
	}
	if (v->style.h_per > 0.0)
	{
	    float height = rel_h;
	    if (view->style.display == LD_FLEX && view->style.flexdir == FD_COL)
	    {
		height  = floorf(rel_h / rem_h);
		frame.y = act_y;
		act_y += height;
		rem_h -= 1;
		rel_h -= height;
	    }

	    frame.h = height * v->style.h_per;
	}
	if (v->style.margin == INT_MAX || view->style.cjustify == JC_CENTER)
	{
	    frame.x = (view->frame.local.w / 2.0) - (frame.w / 2.0);
	}
	if (view->style.itemalign == IA_CENTER)
	{
	    frame.y = (view->frame.local.h / 2.0) - (frame.h / 2.0);
	}
	if (v->style.margin_top < INT_MAX)
	{
	    frame.y += v->style.margin_top;
	    frame.h -= v->style.margin_top;
	}
	if (v->style.margin_left < INT_MAX)
	{
	    frame.x += v->style.margin_left;
	    frame.w -= v->style.margin_left;
	}
	if (v->style.margin_right < INT_MAX)
	{
	    frame.w -= v->style.margin_right;
	}
	if (v->style.margin_bottom < INT_MAX)
	{
	    frame.h -= v->style.margin_bottom;
	}
	if (v->style.top < INT_MAX)
	{
	    frame.y = v->style.top;
	}
	if (v->style.left < INT_MAX)
	{
	    frame.x = v->style.left;
	}
	if (v->style.right < INT_MAX)
	{
	    frame.x = view->frame.local.w - frame.w - v->style.right;
	}
	if (v->style.bottom < INT_MAX)
	{
	    frame.y = view->frame.local.h - frame.h - v->style.bottom;
	}
	ku_view_set_frame(v, frame);
    }

    for (int i = 0; i < view->views->length; i++)
    {
	ku_view_t* v = view->views->data[i];
	ku_view_layout(v);
    }
}

void ku_view_draw_delimiter(ku_view_t* view)
{
    if (view->parent)
    {
	ku_view_draw_delimiter(view->parent);
	uint32_t index = vec_index_of_data(view->parent->views, view);
	if (index == view->parent->views->length - 1)
	    printf("    ");
	else
	    printf("│   ");
    }
}

void ku_view_desc(void* pointer, int level)
{
    ku_view_t* view = (ku_view_t*) pointer;
    printf("view %s", view->id);
}

void ku_view_describe(void* pointer, int level)
{
    ku_view_t* view = (ku_view_t*) pointer;

    char* arrow = "├── ";
    if (view->parent)
    {
	ku_view_draw_delimiter(view->parent);
	if (vec_index_of_data(view->parent->views, view) == view->parent->views->length - 1) arrow = "└── ";
	printf("%s", arrow);
    }

    printf("%s [x:%.2f y:%.2f w:%.2f h:%.2f tx:%i eh:%i tg:%i rc:%zu]\n", view->id, view->frame.local.x, view->frame.local.y, view->frame.local.w, view->frame.local.h, view->texture.page, view->handler != NULL, view->tex_gen != NULL, mem_retaincount(view));

    for (int i = 0; i < view->views->length; i++) ku_view_describe(view->views->data[i], level + 1);
}

void ku_view_desc_style(vstyle_t l)
{
    printf(
	"position %i\n"
	"display %i\n"
	"flexdir %i\n"
	"itemalign %i\n"
	"cjustify %i\n"
	"w_per %f\n"
	"h_per %f\n"
	"width %i\n"
	"height %i\n"
	"margin %i\n"
	"margin_top %i\n"
	"margin_left %i\n"
	"margin_right %i\n"
	"margin_bottom %i\n"
	"top %i\n"
	"left %i\n"
	"right %i\n"
	"bottom %i\n"
	"border_radius %i\n"
	"background_color %x\n"
	"background_image %s\n"
	"font_family %s\n"
	"font_size %f\n"
	"color %x\n"
	"text_align %i\n"
	"line_height %f\n"
	"word_wrap %i\n",
	l.position,
	l.display,
	l.flexdir,
	l.itemalign,
	l.cjustify,
	l.w_per,
	l.h_per,
	l.width,
	l.height,
	l.margin,
	l.margin_top,
	l.margin_left,
	l.margin_right,
	l.margin_bottom,
	l.top,
	l.left,
	l.right,
	l.bottom,
	l.border_radius,
	l.background_color,
	l.background_image == NULL ? "" : l.background_image,
	l.font_family == NULL ? "" : l.font_family,
	l.font_size,
	l.color,
	l.text_align,
	l.line_height,
	l.word_wrap);
}

#endif
