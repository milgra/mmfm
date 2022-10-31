#ifndef ku_gen_type_h
#define ku_gen_type_h

#include "vh_button.c"
#include "vh_slider.c"
#include "zc_vector.c"

void ku_gen_type_apply(vec_t* views, void (*button_event)(vh_button_event_t), void (*slider_event)(vh_slider_event_t));

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_view.c"
#include "tg_css.c"
#include "tg_text.c"

void ku_gen_type_apply(vec_t* views, void (*button_event)(vh_button_event_t), void (*slider_event)(vh_slider_event_t))
{
    for (int index = 0; index < views->length; index++)
    {
	ku_view_t* view = views->data[index];

	if (view->type && strcmp(view->type, "label") == 0)
	{
	    printf("label : %s\n", view->text);
	    tg_text_add(view);
	    tg_text_set1(view, view->text);
	}
	else if (view->style.background_color > 0)
	{
	    tg_css_add(view);
	}
	else if (view->style.background_image)
	{
	    tg_css_add(view);
	}

	if (view->type && strcmp(view->type, "button") == 0)
	{
	    vh_button_add(view, VH_BUTTON_NORMAL, button_event);
	}
	else if (view->type && strcmp(view->type, "slider") == 0)
	{
	    vh_slider_add(view, slider_event);
	}
    }
}

#endif
