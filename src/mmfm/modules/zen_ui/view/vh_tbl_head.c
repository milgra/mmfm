#ifndef vh_tbl_head_h
#define vh_tbl_head_h

#include "view.c"
#include "zc_vector.c"

typedef struct _vh_tbl_head_t
{
    void (*head_create)(view_t* view, void* userdata);
} vh_tbl_head_t;

void vh_tbl_head_attach(
    view_t* view,
    void (*head_create)(view_t* hview, void* userdata),
    vec_t* fields,
    void*  userdata);

#endif

#if __INCLUDE_LEVEL__ == 0

#endif
