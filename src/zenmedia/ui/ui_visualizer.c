#ifndef ui_visualizer_h
#define ui_visualizer_h

#include "view.c"

void ui_visualizer_attach(view_t* baseview);
void ui_visualizer_detach();
void ui_visualizer_update();
void ui_visualizer_update_video();
void ui_visualizer_show_image(bm_t* bm);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "player.c"
#include "vh_anim.c"
#include "vh_button.c"
#include "vh_roll.c"
#include "zc_callback.c"
#include "zc_graphics.c"

struct vizualizer_t
{
  int     visu;
  view_t* visuleft;
  view_t* visuright;
  view_t* visuvideo;
  /* view_t* visuleftbtn; */
  /* view_t* visurightbtn; */
  /* view_t* visuleftbtnbck; */
  /* view_t* visurightbtnbck; */
} uiv;

void ui_visualizer_on_roll_in(void* userdata, void* data);
void ui_visualizer_on_roll_out(void* userdata, void* data);
void ui_visualizer_on_button_down(void* userdata, void* data);

void ui_visualizer_attach(view_t* baseview)
{
  uiv.visuleft  = view_get_subview(baseview, "visuleft");
  uiv.visuright = view_get_subview(baseview, "visuright");
  uiv.visuvideo = view_get_subview(baseview, "visuvideo");

  /* uiv.visuleftbtn     = view_get_subview(uiv.visuleft, "visuleft_btn"); */
  /* uiv.visurightbtn    = view_get_subview(uiv.visuright, "visuright_btn"); */
  /* uiv.visuleftbtnbck  = view_get_subview(uiv.visuleft, "visuleft_btn_bck"); */
  /* uiv.visurightbtnbck = view_get_subview(uiv.visuright, "visuright_btn_bck"); */

  /* vh_anim_add(uiv.visuleftbtnbck); */
  /* vh_anim_add(uiv.visurightbtnbck); */

  // visualise roll over

  /* cb_t* cb_btn_press     = cb_new(ui_visualizer_on_button_down, NULL); // REL 0 */
  /* cb_t* cb_roll_in_visu  = cb_new(ui_visualizer_on_roll_in, NULL);     // REL 1 */
  /* cb_t* cb_roll_out_visu = cb_new(ui_visualizer_on_roll_out, NULL);    // REL 2 */

  /* vh_button_add(uiv.visuleftbtn, VH_BUTTON_NORMAL, cb_btn_press); */
  /* vh_button_add(uiv.visurightbtn, VH_BUTTON_NORMAL, cb_btn_press); */

  /* vh_roll_add(uiv.visuleft, cb_roll_in_visu, cb_roll_out_visu); */
  /* vh_roll_add(uiv.visuright, cb_roll_in_visu, cb_roll_out_visu); */

  /* vh_anim_alpha(uiv.visuleftbtnbck, 1.0, 0.0, 10, AT_LINEAR); */
  /* vh_anim_alpha(uiv.visurightbtnbck, 1.0, 0.0, 10, AT_LINEAR); */

  /* REL(cb_btn_press);     // REL 0 */
  /* REL(cb_roll_in_visu);  // REL 1 */
  /* REL(cb_roll_out_visu); // REL 2 */
}

void ui_visualizer_detach()
{
}

void ui_visualizer_update()
{
  if (uiv.visu)
  {
    player_draw_rdft(0, uiv.visuleft->texture.bitmap, 3);
    player_draw_rdft(1, uiv.visuright->texture.bitmap, 3);
  }
  else
  {
    player_draw_waves(0, uiv.visuleft->texture.bitmap, 3);
    player_draw_waves(1, uiv.visuright->texture.bitmap, 3);
  }

  uiv.visuleft->texture.changed  = 1;
  uiv.visuright->texture.changed = 1;
}

void ui_visualizer_update_video()
{
  /* player_draw_video(uiv.visuvideo->texture.bitmap, 3); */
  /* uiv.visuvideo->texture.changed = 1; */
}

void ui_visualizer_show_image(bm_t* bm)
{
  if (uiv.visuvideo->texture.bitmap != NULL)
  {
    gfx_insert(uiv.visuvideo->texture.bitmap, bm, 0, 0);
    uiv.visuvideo->texture.changed = 1;
  }
}

/* void ui_visualizer_on_roll_in(void* userdata, void* data) */
/* { */
/*   vh_anim_alpha(uiv.visuleftbtnbck, 0.0, 1.0, 10, AT_LINEAR); */
/*   vh_anim_alpha(uiv.visurightbtnbck, 0.0, 1.0, 10, AT_LINEAR); */
/* } */

/* void ui_visualizer_on_roll_out(void* userdata, void* data) */
/* { */
/*   vh_anim_alpha(uiv.visuleftbtnbck, 1.0, 0.0, 10, AT_LINEAR); */
/*   vh_anim_alpha(uiv.visurightbtnbck, 1.0, 0.0, 10, AT_LINEAR); */
/* } */

void ui_visualizer_change_visu()
{
  uiv.visu = 1 - uiv.visu;
}

/* void ui_visualizer_on_button_down(void* userdata, void* data) */
/* { */
/*   char* id = ((view_t*)data)->id; */

/*   if (strcmp(id, "visuright_btn") == 0) ui_visualizer_change_visu(); */
/*   if (strcmp(id, "visuleft_btn") == 0) ui_visualizer_change_visu(); */
/* } */

#endif
