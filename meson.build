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
rt = cc.find_library('rt')
gl = cc.find_library('GL')
glew = cc.find_library('GLEW')
sdl2 = cc.find_library('SDL2')
avutil = cc.find_library('avutil')
avcodec = cc.find_library('avcodec')
avdevice = cc.find_library('avdevice')
avformat = cc.find_library('avformat')
avfilter = cc.find_library('avfilter')
swresample = cc.find_library('swresample')
swscale = cc.find_library('swscale')
pthread = cc.find_library('pthread')
jpeg = cc.find_library('jpeg')


libdir = meson.current_source_dir() + './lib/'

mupdf = cc.find_library('mupdf', dirs : libdir)
mupdfthird = cc.find_library('mupdf-third', dirs : libdir)

mmfm_version = '"@0@"'.format(meson.project_version())
add_project_arguments('-DMMFM_VERSION=@0@'.format(mmfm_version), language: 'c')

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
endif

mmfm_inc = include_directories(
	'/usr/include',
	'/usr/include/GL',
	'/usr/include/SDL2',
	'src/modules',
	'src/modules/zen_core',
	'src/modules/zen_math',
	'src/modules/zen_wm',
	'src/modules/zen_media_player',
	'src/modules/zen_media_transcoder',
	'src/modules/zen_ui',
	'src/modules/zen_ui/gl',
	'src/modules/zen_ui/view',
	'src/modules/zen_ui/text',
	'src/modules/zen_ui/html',
	'src/mmfm',
	'src/mmfm/ui')


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
	       'src/mmfm/files.c',
	       'src/mmfm/kvlist.c',
	       'src/mmfm/library.c',
	       'src/mmfm/mmfm.c',
	       'src/mmfm/selection.c',
	       'src/mmfm/visible.c',

	       'src/modules/zen_core/zc_bitmap.c',
	       'src/modules/zen_core/zc_callback.c',
	       'src/modules/zen_core/zc_channel.c',
	       'src/modules/zen_core/zc_cstring.c',
	       'src/modules/zen_core/zc_cstrpath.c',
	       'src/modules/zen_core/zc_graphics.c',
	       'src/modules/zen_core/zc_log.c',
	       'src/modules/zen_core/zc_map.c',
	       'src/modules/zen_core/zc_memory.c',
	       'src/modules/zen_core/zc_string.c',
	       'src/modules/zen_core/zc_strutil.c',
	       'src/modules/zen_core/zc_texmap.c',
	       'src/modules/zen_core/zc_vector.c',

	       'src/modules/zen_math/zm_math2.c',
	       'src/modules/zen_math/zm_math3.c',
	       'src/modules/zen_math/zm_math4.c',

	       'src/modules/zen_media_player/clock.c',
	       'src/modules/zen_media_player/codec.c',
	       'src/modules/zen_media_player/decoder.c',
	       'src/modules/zen_media_player/frame.c',
	       'src/modules/zen_media_player/options.c',
	       'src/modules/zen_media_player/packet.c',
	       'src/modules/zen_media_player/player.c',
	       'src/modules/zen_media_player/render.c',
	       'src/modules/zen_media_player/strcomm.c',
	       'src/modules/zen_media_player/stream.c',
	       'src/modules/zen_media_player/video.c',

	       'src/modules/zen_media_transcoder/coder.c',

	       'src/modules/zen_wm/wm_connector.c',
	       'src/modules/zen_wm/wm_event.c',

	       'src/modules/zen_ui/gl/gl_connector.c',
	       'src/modules/zen_ui/gl/gl_floatbuffer.c',
	       'src/modules/zen_ui/gl/gl_shader.c',
	       'src/modules/zen_ui/html/html.c',
	       'src/modules/zen_ui/text/text.c',
	       'src/modules/zen_ui/view/tg_css.c',
	       'src/modules/zen_ui/view/tg_knob.c',
	       'src/modules/zen_ui/view/tg_picker.c',
	       'src/modules/zen_ui/view/tg_text.c',
	       'src/modules/zen_ui/view/vh_anim.c',
	       'src/modules/zen_ui/view/vh_button.c',
	       'src/modules/zen_ui/view/vh_drag.c',
	       'src/modules/zen_ui/view/vh_key.c',
	       'src/modules/zen_ui/view/vh_knob.c',
	       'src/modules/zen_ui/view/vh_list.c',
	       'src/modules/zen_ui/view/vh_list_cell.c',
	       'src/modules/zen_ui/view/vh_list_head.c',
	       'src/modules/zen_ui/view/vh_list_item.c',
	       'src/modules/zen_ui/view/vh_picker.c',
	       'src/modules/zen_ui/view/vh_roll.c',
	       'src/modules/zen_ui/view/vh_sbar.c',
	       'src/modules/zen_ui/view/vh_textinput.c',
	       'src/modules/zen_ui/view/vh_textinput_scroller.c',
	       'src/modules/zen_ui/view/vh_touch.c',
	       'src/modules/zen_ui/view/view.c',
	       'src/modules/zen_ui/view/view_generator.c',
	       'src/modules/zen_ui/view/view_layout.c',
	       'src/modules/zen_ui/view/views.c',
	       'src/modules/zen_ui/ui_compositor.c',
	       'src/modules/zen_ui/ui_generator.c',
	       'src/modules/zen_ui/ui_manager.c']

mmfm_dependencies = [rt, freetype, math,gl,glew,sdl2,avutil,avcodec,avdevice,avformat,avfilter,swresample,swscale,pthread,jpeg,mupdf,mupdfthird]

executable(
  'mmfm',
  com_sources,
  include_directories: [mmfm_inc],
  dependencies: mmfm_dependencies,
  install: true,
)