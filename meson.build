project(
  'mmfm',
  'c',
  version: '0.1',
  license: 'MIT',
  default_options: ['c_std=c99']
)

cc = meson.get_compiler('c')

freetype = dependency('freetype2')
math = cc.find_library('m')
z = cc.find_library('z')
rt = cc.find_library('rt')
gl = cc.find_library('GL')
glew = cc.find_library('GLEW')
sdl2 = cc.find_library('SDL2')
swscale = cc.find_library('swscale')
pthread = cc.find_library('pthread')
jpeg = cc.find_library('jpeg')
jbig = cc.find_library('jbig2dec')
ojpeg = cc.find_library('openjp2')
hb = cc.find_library('harfbuzz')
gumbo = cc.find_library('gumbo')
avutil = cc.find_library('avutil')
avcodec = cc.find_library('avcodec')
avdevice = cc.find_library('avdevice')
avformat = cc.find_library('avformat')
avfilter = cc.find_library('avfilter')
swresample = cc.find_library('swresample')

libdir = meson.current_source_dir() + './lib/'

mupdf = cc.find_library('mupdf', dirs : libdir)
mupdfthird = cc.find_library('mupdf-third', dirs : libdir)

mmfm_version = '"@0@"'.format(meson.project_version())
add_project_arguments('-DMMFM_VERSION=@0@'.format(mmfm_version), language: 'c')
add_project_arguments('-Wno-unused-but-set-variable', language: 'c')

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
endif

mmfm_inc = include_directories(
	'/usr/include',
	'/usr/include/GL',
	'/usr/include/SDL2',
	'src/modules',
	'src/modules/zen_math',
	'src/modules/zen_gfx',
	'src/modules/zen_core',
	'src/mmfm/modules/zen_wm',
	'src/mmfm/modules/zen_media_player',
	'src/mmfm/modules/zen_media_transcoder',
	'src/mmfm/modules/zen_ui',
	'src/mmfm/modules/zen_ui/gl',
	'src/mmfm/modules/zen_ui/view',
	'src/mmfm/modules/zen_ui/html',
	'src/mmfm',
	'src/mmfm/ui',
	'src/mmfm/utils',
	'inc')


com_sources = ['src/mmfm/ui/column.c',
	       'src/mmfm/ui/textlist.c',
	       'src/mmfm/ui/ui.c',
	       'src/mmfm/ui/ui_about_popup.c',
	       'src/mmfm/ui/ui_activity_popup.c',
	       'src/mmfm/ui/ui_alert_popup.c',
	       'src/mmfm/ui/ui_cliplist.c',
	       'src/mmfm/ui/ui_decision_popup.c',
	       'src/mmfm/ui/ui_filelist.c',
	       'src/mmfm/ui/ui_filter_bar.c',
	       'src/mmfm/ui/ui_filter_popup.c',
	       'src/mmfm/ui/ui_inputfield_popup.c',
	       'src/mmfm/ui/ui_lib_init_popup.c',
	       'src/mmfm/ui/ui_meta_view.c',
	       'src/mmfm/ui/ui_play_controls.c',
	       'src/mmfm/ui/ui_popup_switcher.c',
	       'src/mmfm/ui/ui_settings_popup.c',
	       'src/mmfm/ui/ui_song_infos.c',
	       'src/mmfm/ui/ui_song_menu_popup.c',
	       'src/mmfm/ui/ui_visualizer.c',
	       'src/mmfm/callbacks.c',
	       'src/mmfm/config.c',
	       'src/mmfm/evrecorder.c',
	       'src/mmfm/filemanager.c',
	       'src/mmfm/kvlist.c',
	       'src/mmfm/mmfm.c',
	       'src/mmfm/selection.c',
	       'src/mmfm/visible.c',
	       'src/mmfm/pdf.c',
	       'src/mmfm/utils/bm_rgba_util.c',
	       'src/mmfm/utils/cstr_util.c',
	       'src/mmfm/utils/str_util.c',

	       'src/modules/zen_core/zc_bm_rgba.c',
	       'src/modules/zen_core/zc_callback.c',
	       'src/modules/zen_core/zc_channel.c',
	       'src/modules/zen_core/zc_cstring.c',
	       'src/modules/zen_core/zc_path.c',
	       'src/modules/zen_core/zc_log.c',
	       'src/modules/zen_core/zc_map.c',
	       'src/modules/zen_core/zc_memory.c',
	       'src/modules/zen_core/zc_number.c',
	       'src/modules/zen_core/zc_string.c',
	       'src/modules/zen_core/zc_vector.c',
	       'src/modules/zen_core/zc_wrapper.c',

	       'src/modules/zen_math/zc_mat3.c',
	       'src/modules/zen_math/zc_mat4.c',
	       'src/modules/zen_math/zc_vec2.c',
	       'src/modules/zen_math/zc_vec3.c',
	       'src/modules/zen_math/zc_vec4.c',
	       'src/modules/zen_math/zc_util2.c',
	       'src/modules/zen_math/zc_util3.c',

	       'src/modules/zen_gfx/zc_draw.c',
	       'src/modules/zen_gfx/zc_text.c',

	       'src/mmfm/modules/zen_media_player/clock.c',
	       'src/mmfm/modules/zen_media_player/codec.c',
	       'src/mmfm/modules/zen_media_player/decoder.c',
	       'src/mmfm/modules/zen_media_player/frame.c',
	       'src/mmfm/modules/zen_media_player/options.c',
	       'src/mmfm/modules/zen_media_player/packet.c',
	       'src/mmfm/modules/zen_media_player/player.c',
	       'src/mmfm/modules/zen_media_player/render.c',
	       'src/mmfm/modules/zen_media_player/strcomm.c',
	       'src/mmfm/modules/zen_media_player/stream.c',
	       'src/mmfm/modules/zen_media_player/video.c',

	       'src/mmfm/modules/zen_media_transcoder/coder.c',

	       'src/mmfm/modules/zen_wm/wm_connector.c',
	       'src/mmfm/modules/zen_wm/wm_event.c',

	       'src/mmfm/modules/zen_ui/gl/gl_connector.c',
	       'src/mmfm/modules/zen_ui/gl/gl_floatbuffer.c',
	       'src/mmfm/modules/zen_ui/gl/gl_shader.c',
	       'src/mmfm/modules/zen_ui/html/html.c',
	       'src/mmfm/modules/zen_ui/view/tg_css.c',
	       'src/mmfm/modules/zen_ui/view/tg_knob.c',
	       'src/mmfm/modules/zen_ui/view/tg_picker.c',
	       'src/mmfm/modules/zen_ui/view/tg_text.c',
	       'src/mmfm/modules/zen_ui/view/vh_anim.c',
	       'src/mmfm/modules/zen_ui/view/vh_button.c',
	       'src/mmfm/modules/zen_ui/view/vh_drag.c',
	       'src/mmfm/modules/zen_ui/view/vh_key.c',
	       'src/mmfm/modules/zen_ui/view/vh_knob.c',
	       'src/mmfm/modules/zen_ui/view/vh_list.c',
	       'src/mmfm/modules/zen_ui/view/vh_list_cell.c',
	       'src/mmfm/modules/zen_ui/view/vh_list_head.c',
	       'src/mmfm/modules/zen_ui/view/vh_list_item.c',
	       'src/mmfm/modules/zen_ui/view/vh_picker.c',
	       'src/mmfm/modules/zen_ui/view/vh_roll.c',
	       'src/mmfm/modules/zen_ui/view/vh_sbar.c',
	       'src/mmfm/modules/zen_ui/view/vh_textinput.c',
	       'src/mmfm/modules/zen_ui/view/vh_textinput_scroller.c',
	       'src/mmfm/modules/zen_ui/view/vh_touch.c',
	       'src/mmfm/modules/zen_ui/view/view.c',
	       'src/mmfm/modules/zen_ui/view/view_generator.c',
	       'src/mmfm/modules/zen_ui/view/view_layout.c',
	       'src/mmfm/modules/zen_ui/view/views.c',
	       'src/mmfm/modules/zen_ui/ui_compositor.c',
	       'src/mmfm/modules/zen_ui/ui_generator.c',
	       'src/mmfm/modules/zen_ui/ui_manager.c',
	       'src/mmfm/modules/zen_ui/ui_texmap.c'
	       ]

mmfm_dependencies = [rt, freetype, math,gl,glew,sdl2,avutil,avcodec,avdevice,avformat,avfilter,swresample,swscale,pthread,jpeg,ojpeg,jbig,hb,z,gumbo,mupdf,mupdfthird]

executable(
  'mmfm',
  com_sources,
  include_directories: [mmfm_inc],
  dependencies: mmfm_dependencies,
  install: true,
)