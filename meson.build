project(
  'mmfm',
  'c',
  version: '0.1',
  license: 'MIT',
  default_options: ['c_std=gnu99']
)

cc = meson.get_compiler('c')

freetype = cc.find_library('freetype',static:false)
math = cc.find_library('m',static:false)
z = cc.find_library('z',static:false)
bz2 = cc.find_library('bz2',static:false)
rt = cc.find_library('rt',static:false)
gl = cc.find_library('GL',static:false)
glew = cc.find_library('GLEW',static:false)
sdl2 = cc.find_library('SDL2',static:false)
swscale = cc.find_library('swscale',static:false)
pthread = cc.find_library('pthread',static:false)
jpeg = cc.find_library('jpeg',static:false)
jbig = cc.find_library('jbig2dec',static:false)
ojpeg = cc.find_library('openjp2',static:false)
hb = cc.find_library('harfbuzz',static:false)
gumbo = cc.find_library('gumbo',static:false)
avutil = cc.find_library('avutil',static:false)
avcodec = cc.find_library('avcodec',static:false)
avdevice = cc.find_library('avdevice',static:false)
avformat = cc.find_library('avformat',static:false)
avfilter = cc.find_library('avfilter',static:false)
swresample = cc.find_library('swresample',static:false)

# for ffmpeg
va = cc.find_library('va',static:false)
x11 = cc.find_library('X11',static:false)
lvadrm = cc.find_library('va-drm',static:false)
lvax11 = cc.find_library('va-x11',static:false)
lzma = cc.find_library('lzma',static:false)
lvdpau = cc.find_library('vdpau',static:false)

mupdf = cc.find_library('mupdf')
mupdfthird = cc.find_library('mupdf-third')

mmfm_dependencies = [rt,freetype, math,gl,glew,sdl2,avutil,avcodec,avdevice,avformat,avfilter,swresample,swscale,pthread,jpeg,ojpeg,jbig,hb,z,bz2,x11,va,gumbo,mupdf,mupdfthird,lzma,lvadrm,lvax11,lvdpau]

mmfm_version = '"@0@"'.format(meson.project_version())
pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'mmfm'
add_project_arguments('-DPKG_DATADIR="' + pkg_datadir + '"',
		      '-DMMFM_VERSION=@0@'.format(mmfm_version),
		      language: 'c')
add_project_arguments('-Wno-unused-but-set-variable', language: 'c')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG',language: 'c')
endif

mmfm_inc = include_directories(
	'/usr/include',
	'/usr/include/GL',
	'/usr/include/SDL2',
	'/usr/include/mupdf',
	'/usr/include/freetype2',
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
	       'src/mmfm/viewer/decoder.c',
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
	       'src/mmfm/zen_ui/view/tg_scaledimg.c',
	       'src/mmfm/zen_ui/view/vh_anim.c',
	       'src/mmfm/zen_ui/view/vh_button.c',
	       'src/mmfm/zen_ui/view/vh_drag.c',
	       'src/mmfm/zen_ui/view/vh_key.c',
	       'src/mmfm/zen_ui/view/vh_knob.c',
	       'src/mmfm/zen_ui/view/vh_picker.c',
	       'src/mmfm/zen_ui/view/vh_roll.c',
	       'src/mmfm/zen_ui/view/vh_textinput.c',
	       'src/mmfm/zen_ui/view/vh_touch.c',
	       'src/mmfm/zen_ui/view/vh_cv_body.c',
	       'src/mmfm/zen_ui/view/vh_cv_scrl.c',
	       'src/mmfm/zen_ui/view/vh_cv_evnt.c',
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

pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'mmfm'
install_data('res/html/main.html', install_dir : pkg_datadir / 'html' )
install_data('res/html/main.css', install_dir : pkg_datadir / 'html' )
install_data('res/img/close.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/max.png', install_dir : pkg_datadir / 'img' )

executable(
  'mmfm',
  com_sources,
  include_directories: [mmfm_inc],
  dependencies: mmfm_dependencies,
  install: true,
)