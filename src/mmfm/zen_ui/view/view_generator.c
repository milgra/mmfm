#ifndef view_gen_h
#define view_gen_h

#include "view.c"
#include "zc_map.c"
#include "zc_vector.c"

vec_t* view_gen_load(char* htmlpath, char* csspath, char* respath);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "css.c"
#include "cstr_util.c"
#include "html.c"
#include "tg_css.c"
#include "vh_button.c"
#include "zc_callback.c"
#include "zc_log.c"
#include <limits.h>

void view_gen_apply_style(view_t* view, map_t* style, char* respath)
{
    vec_t* keys = VNEW(); // REL 0
    map_keys(style, keys);

    for (int index = 0; index < keys->length; index++)
    {
	char* key = keys->data[index];
	char* val = MGET(style, key);

	if (strcmp(key, "background-color") == 0)
	{
	    int color                    = (int) strtol(val + 1, NULL, 16);
	    view->style.background_color = color;
	    tg_css_add(view);
	}
	else if (strcmp(key, "background-image") == 0)
	{
	    if (strstr(val, "url") != NULL)
	    {
		char* url = CAL(sizeof(char) * strlen(val), NULL, cstr_describe); // REL 0
		memcpy(url, val + 5, strlen(val) - 7);
		char* imagepath              = cstr_new_format(100, "%s/img/%s", respath, url);
		view->style.background_image = imagepath;
		REL(url); // REL 0
		tg_css_add(view);
	    }
	}
	else if (strcmp(key, "font-family") == 0)
	{
	    char* url = CAL(sizeof(char) * strlen(val), NULL, cstr_describe); // REL 0
	    memcpy(url, val + 1, strlen(val) - 2);
	    view->style.font_family = url;
	}
	else if (strcmp(key, "color") == 0)
	{
	    int color         = (int) strtol(val + 1, NULL, 16);
	    view->style.color = color;
	}
	else if (strcmp(key, "font-size") == 0)
	{
	    float size            = atof(val);
	    view->style.font_size = size;
	}
	else if (strcmp(key, "line-height") == 0)
	{
	    float size              = atof(val);
	    view->style.line_height = size;
	}
	else if (strcmp(key, "word-wrap") == 0)
	{
	    if (strstr(val, "normal") != NULL) view->style.word_wrap = 0;
	    if (strstr(val, "break-word") != NULL) view->style.word_wrap = 1;
	    if (strstr(val, "initial") != NULL) view->style.word_wrap = 2;
	    if (strstr(val, "inherit") != NULL) view->style.word_wrap = 3;
	}
	else if (strcmp(key, "text-align") == 0)
	{
	    if (strstr(val, "left") != NULL) view->style.text_align = 0;
	    if (strstr(val, "center") != NULL) view->style.text_align = 1;
	    if (strstr(val, "right") != NULL) view->style.text_align = 2;
	    if (strstr(val, "justify") != NULL) view->style.text_align = 3;
	}
	else if (strcmp(key, "width") == 0)
	{
	    if (strstr(val, "%") != NULL)
	    {
		int per           = atoi(val);
		view->style.w_per = (float) per / 100.0;
	    }
	    else if (strstr(val, "px") != NULL)
	    {
		int pix             = atoi(val);
		view->style.width   = pix;
		view->frame.local.w = pix;
	    }
	}
	else if (strcmp(key, "height") == 0)
	{
	    if (strstr(val, "%") != NULL)
	    {
		int per           = atoi(val);
		view->style.h_per = (float) per / 100.0;
	    }
	    else if (strstr(val, "px") != NULL)
	    {
		int pix             = atoi(val);
		view->style.height  = pix;
		view->frame.local.h = pix;
	    }
	}
	else if (strcmp(key, "display") == 0)
	{
	    if (strcmp(val, "flex") == 0)
	    {
		view->style.display = LD_FLEX;
		view->exclude       = 1;
	    }
	    if (strcmp(val, "none") == 0)
		view->exclude = 1;
	}
	else if (strcmp(key, "overflow") == 0)
	{
	    if (strcmp(val, "hidden") == 0)
		view->style.masked = 1;
	}
	else if (strcmp(key, "flex-direction") == 0)
	{
	    if (strcmp(val, "column") == 0)
		view->style.flexdir = FD_COL;
	    else
		view->style.flexdir = FD_ROW;
	}
	else if (strcmp(key, "margin") == 0)
	{
	    if (strcmp(val, "auto") == 0)
	    {
		view->style.margin = INT_MAX;
	    }
	    else if (strstr(val, "px") != NULL)
	    {
		int pix                   = atoi(val);
		view->style.margin        = pix;
		view->style.margin_top    = pix;
		view->style.margin_left   = pix;
		view->style.margin_right  = pix;
		view->style.margin_bottom = pix;
	    }
	}
	else if (strcmp(key, "top") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix         = atoi(val);
		view->style.top = pix;
	    }
	}
	else if (strcmp(key, "left") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix          = atoi(val);
		view->style.left = pix;
	    }
	}
	else if (strcmp(key, "right") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix           = atoi(val);
		view->style.right = pix;
	    }
	}
	else if (strcmp(key, "bottom") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix            = atoi(val);
		view->style.bottom = pix;
	    }
	}
	else if (strcmp(key, "margin-top") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix                = atoi(val);
		view->style.margin_top = pix;
	    }
	}
	else if (strcmp(key, "margin-left") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix                 = atoi(val);
		view->style.margin_left = pix;
	    }
	}
	else if (strcmp(key, "margin-right") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix                  = atoi(val);
		view->style.margin_right = pix;
	    }
	}
	else if (strcmp(key, "margin-bottom") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix                   = atoi(val);
		view->style.margin_bottom = pix;
	    }
	}
	else if (strcmp(key, "border-radius") == 0)
	{
	    if (strstr(val, "px") != NULL)
	    {
		int pix                   = atoi(val);
		view->style.border_radius = pix;
	    }
	}
	else if (strcmp(key, "box-shadow") == 0)
	{
	    view->style.shadow_blur = atoi(val);
	    char* color             = strstr(val + 1, " ");

	    if (color) view->style.shadow_color = (int) strtol(color + 2, NULL, 16);
	}
	else if (strcmp(key, "align-items") == 0)
	{
	    if (strcmp(val, "center") == 0)
	    {
		view->style.itemalign = IA_CENTER;
	    }
	}
	else if (strcmp(key, "justify-content") == 0)
	{
	    if (strcmp(val, "center") == 0)
	    {
		view->style.cjustify = JC_CENTER;
	    }
	}
	// TODO remove non standard CSS
	else if (strcmp(key, "blocks") == 0)
	{
	    if (strcmp(val, "no") == 0)
	    {
		view->blocks_touch = 0;
	    }
	}
    }
    /* printf("style for %s: ", view->id); */
    /* view_desc_style(view->style); */
    /* printf("\n"); */

    REL(keys);
}

vec_t* view_gen_load(char* htmlpath, char* csspath, char* respath)
{
    char* html = cstr_new_file(htmlpath); // REL 0

    tag_t* view_structure = html_new(html); // REL 2

    map_t* styles = css_new(csspath);

    // create view structure
    vec_t* views = VNEW();
    tag_t* tags  = view_structure;
    while ((*tags).len > 0)
    {
	tag_t t = *tags;
	if (t.id.len > 0)
	{
	    char* id = CAL(sizeof(char) * t.id.len + 1, NULL, cstr_describe); // REL 0

	    memcpy(id, html + t.id.pos + 1, t.id.len);

	    view_t* view = view_new(id, (r2_t){0}); // REL 1

	    VADD(views, view);

	    if (t.level > 0) // add view to paernt
	    {
		view_t* parent = views->data[t.parent];
		// printf("parent %i %i\n", t.parent, views->length);
		view_add_subview(parent, view);
	    }

	    char cssid[100] = {0};
	    snprintf(cssid, 100, "#%s", id);

	    map_t* style = MGET(styles, cssid);
	    if (style)
	    {
		view_gen_apply_style(view, style, respath);
	    }

	    if (t.class.len > 0)
	    {
		char* class = CAL(sizeof(char) * t.class.len + 1, NULL, cstr_describe); // REL 0
		memcpy(class, html + t.class.pos + 1, t.class.len);

		char csscls[100] = {0};
		snprintf(csscls, 100, ".%s", class);

		style = MGET(styles, csscls);
		if (style)
		{
		    view_gen_apply_style(view, style, respath);
		}
		REL(class);
	    }

	    if (t.type.len > 0)
	    {
		char* type = CAL(sizeof(char) * t.type.len + 1, NULL, cstr_describe); // REL 2
		memcpy(type, html + t.type.pos + 1, t.type.len);

		// TODO remove non-standard types
		if (strcmp(type, "button") == 0 && t.onclick.len > 0)
		{
		    // vh_button_add(view, VH_BUTTON_NORMAL, NULL);
		}
		else if (strcmp(type, "checkbox") == 0 && t.onclick.len > 0)
		{
		    char* onclick = CAL(sizeof(char) * t.onclick.len + 1, NULL, cstr_describe); // REL 5
		    memcpy(onclick, html + t.onclick.pos + 1, t.onclick.len);

		    /* cb_t* callback = MGET(callbacks, onclick); */
		    /* if (callback) vh_button_add(view, VH_BUTTON_TOGGLE, callback); */

		    REL(onclick); // REL 5
		}
		REL(type); // REL 2
	    }

	    REL(id);   // REL 0
	    REL(view); // REL 1
	}
	else
	{
	    static int divcnt = 0;
	    char*      divid  = cstr_new_format(10, "div%i", divcnt++);
	    // idless view, probably </div>
	    view_t* view = view_new(divid, (r2_t){0});
	    VADD(views, view);
	    REL(view);
	    REL(divid);
	}
	tags += 1;
    }

    // cleanup

    REL(view_structure); // REL 2

    REL(html); // REL 0

    return views;
}

#endif
