#ifndef ui_visualizer_h
#define ui_visualizer_h

#include "view.c"

void ui_visualizer_open(char* path);
void ui_visualizer_attach(view_t* baseview);
void ui_visualizer_detach();
void ui_visualizer_update();
void ui_visualizer_update_video();
void ui_visualizer_show_image(bm_rgba_t* bm);
void ui_visualizer_show_pdf(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "pdf.c"
#include "vh_anim.c"
#include "vh_button.c"
#include "vh_roll.c"
#include "viewer.c"
#include "zc_callback.c"
#include "zc_draw.c"
#include "zc_log.c"

struct vizualizer_t
{
    int     visu;
    view_t* visuleft;
    view_t* visuright;
    view_t* visuvideo;
    void*   vs;
} uiv = {0};

void ui_visualizer_on_roll_in(void* userdata, void* data);
void ui_visualizer_on_roll_out(void* userdata, void* data);
void ui_visualizer_on_button_down(void* userdata, void* data);

void ui_visualizer_attach(view_t* baseview)
{
    /* uiv.visuleft  = view_get_subview(baseview, "visuleft"); */
    /* uiv.visuright = view_get_subview(baseview, "visuright"); */
    uiv.visuvideo = view_get_subview(baseview, "visuvideo");
}

void ui_visualizer_detach()
{
}

void ui_visualizer_update()
{
    /* if (uiv.visu) */
    /* { */
    /*   player_draw_rdft(0, uiv.visuleft->texture.bitmap, 3); */
    /*   player_draw_rdft(1, uiv.visuright->texture.bitmap, 3); */
    /* } */
    /* else */
    /* { */
    /*   player_draw_waves(0, uiv.visuleft->texture.bitmap, 3); */
    /*   player_draw_waves(1, uiv.visuright->texture.bitmap, 3); */
    /* } */

    /* uiv.visuleft->texture.changed  = 1; */
    /* uiv.visuright->texture.changed = 1; */
}

void ui_visualizer_open(char* path)
{
    if (uiv.vs)
    {
	viewer_close(uiv.vs);
	uiv.vs = NULL;
    }
    else uiv.vs = viewer_open(path);
}

void ui_visualizer_update_video()
{
    // player_draw_video(uiv.visuvideo->texture.bitmap, 3);

    if (uiv.vs)
    {
	double rem;
	video_refresh(uiv.vs, &rem, uiv.visuvideo->texture.bitmap);
	uiv.visuvideo->texture.changed = 1;
    }
}

void ui_visualizer_show_image(bm_rgba_t* bm)
{
    if (uiv.visuvideo->texture.bitmap != NULL)
    {
	gfx_insert(uiv.visuvideo->texture.bitmap, bm, 0, 0);
	uiv.visuvideo->texture.changed = 1;
    }
}

void ui_visualizer_show_pdf(char* path)
{
    bm_rgba_t* pdfbmp = pdf_render(path);
    ui_visualizer_show_image(pdfbmp);
    zc_log_debug("SHOW PDF");
    REL(pdfbmp);
}

void ui_visualizer_change_visu()
{
    uiv.visu = 1 - uiv.visu;
}

#endif
