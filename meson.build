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
bz2 = cc.find_library('bz2')
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

# for ffmpeg
va = cc.find_library('va')
x11 = cc.find_library('X11')
lvadrm = cc.find_library('va-drm')
lvax11 = cc.find_library('va-x11')
lzma = cc.find_library('lzma')
lvdpau = cc.find_library('vdpau')

mupdf = cc.find_library('mupdf')
mupdfthird = cc.find_library('mupdf-third')

mmfm_dependencies = [rt, freetype, math,gl,glew,sdl2,avutil,avcodec,avdevice,avformat,avfilter,swresample,swscale,pthread,jpeg,ojpeg,jbig,hb,z,bz2,x11,va,gumbo,mupdf,mupdfthird,lzma,lvadrm,lvax11,lvdpau]

mmfm_version = '"@0@"'.format(meson.project_version())
add_project_arguments('-DMMFM_VERSION=@0@'.format(mmfm_version), language: 'c')
add_project_arguments('-Wno-unused-but-set-variable', language: 'c')

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'c')
#  add_project_arguments('-H', language : 'c')
endif

mmfm_inc = include_directories(
	'/usr/include',
	'/usr/include/GL',
	'/usr/include/SDL2',
	'/usr/include/mupdf',
	'src/modules',
	'src/modules/zen_math',
	'src/modules/zen_gfx',
	'src/modules/zen_core',
	'src/mmfm/zen_wm',
	'src/mmfm/viewer',
	'src/mmfm/coder',
	'src/mmfm/zen_ui',
	'src/mmfm/zen_ui/gl',
	'src/mmfm/zen_ui/view',
	'src/mmfm/zen_ui/html',
	'src/mmfm',
	'src/mmfm/ui',
	'src/mmfm/utils')


com_sources = ['src/mmfm/ui/ui.c',
	       'src/mmfm/ui/ui_visualizer.c',
	       'src/mmfm/config.c',
	       'src/mmfm/evrecorder.c',
	       'src/mmfm/filemanager.c',
	       'src/mmfm/kvlist.c',
	       'src/mmfm/mmfm.c',
	       'src/mmfm/fontconfig.c',
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
	       'src/modules/zen_core/zc_time.c',
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

	       'src/mmfm/coder/coder.c',
	       'src/mmfm/viewer/packetqueue.c',
	       'src/mmfm/viewer/framequeue.c',
	       'src/mmfm/viewer/clock.c',
	       'src/mmfm/viewer/viewer.c',

	       'src/mmfm/zen_wm/wm_connector.c',
	       'src/mmfm/zen_wm/wm_event.c',

	       'src/mmfm/zen_ui/gl/gl_connector.c',
	       'src/mmfm/zen_ui/gl/gl_floatbuffer.c',
	       'src/mmfm/zen_ui/gl/gl_shader.c',
	       'src/mmfm/zen_ui/html/html.c',
	       'src/mmfm/zen_ui/html/css.c',
	       'src/mmfm/zen_ui/view/tg_css.c',
	       'src/mmfm/zen_ui/view/tg_knob.c',
	       'src/mmfm/zen_ui/view/tg_picker.c',
	       'src/mmfm/zen_ui/view/tg_text.c',
	       'src/mmfm/zen_ui/view/vh_anim.c',
	       'src/mmfm/zen_ui/view/vh_button.c',
	       'src/mmfm/zen_ui/view/vh_drag.c',
	       'src/mmfm/zen_ui/view/vh_key.c',
	       'src/mmfm/zen_ui/view/vh_knob.c',
	       'src/mmfm/zen_ui/view/vh_picker.c',
	       'src/mmfm/zen_ui/view/vh_roll.c',
	       'src/mmfm/zen_ui/view/vh_textinput.c',
	       'src/mmfm/zen_ui/view/vh_touch.c',
	       'src/mmfm/zen_ui/view/vh_tbl_body.c',
	       'src/mmfm/zen_ui/view/vh_tbl_evnt.c',
	       'src/mmfm/zen_ui/view/vh_tbl_scrl.c',
	       'src/mmfm/zen_ui/view/vh_tbl_head.c',
	       'src/mmfm/zen_ui/view/view.c',
	       'src/mmfm/zen_ui/view/viewgen_html.c',
	       'src/mmfm/zen_ui/view/viewgen_css.c',
	       'src/mmfm/zen_ui/view/viewgen_type.c',
	       'src/mmfm/zen_ui/view/view_layout.c',
	       'src/mmfm/zen_ui/view/views.c',
	       'src/mmfm/zen_ui/ui_compositor.c',
	       'src/mmfm/zen_ui/ui_generator.c',
	       'src/mmfm/zen_ui/ui_manager.c',
	       'src/mmfm/zen_ui/ui_texmap.c',
	       'src/mmfm/zen_ui/ui_table.c',
	       'src/mmfm/zen_ui/ui_util.c']


executable(
  'mmfm',
  com_sources,
  include_directories: [mmfm_inc],
  dependencies: mmfm_dependencies,
  install: true,
)