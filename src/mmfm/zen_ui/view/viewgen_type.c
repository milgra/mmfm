#ifndef viewgen_type_h
#define viewgen_type_h

#include "zc_vector.c"

void viewgen_type_apply(vec_t* views);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "view.c"

void viewgen_type_apply(vec_t* views)
{
    for (int index = 0; index < views->length; index++)
    {
	view_t* view = views->data[index];

	if (view->type)
	{
	    if (strcmp(view->type, "button") == 0)
	    {
	    }
	    else if (strcmp(view->type, "checkbox") == 0)
	    {
	    }
	}
    }
}

#endif
