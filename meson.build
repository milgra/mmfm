project(
  'mmfm',
  'c',
  version: '0.43b',
  license: 'MIT',
  default_options: ['c_std=gnu99']
)

cc = meson.get_compiler('c')

# os specific libs with find library

rt = cc.find_library('rt',static:false)
math = cc.find_library('m',static:false)

# static libs with find library

mupdf = cc.find_library('mupdf')
mupdfthird = cc.find_library('mupdf-third')

# other deps with dependency

z = dependency('zlib',static:false)

png = dependency('libpng',static:false)
freetype = dependency('freetype2',static:false)

gl = dependency('GL',static:false)
egl = dependency('egl',static:false)
wegl = dependency('wayland-egl',static:false)
glew = cc.find_library('GLEW',static:false)
sdl2 = dependency('SDL2',static:false)

pthread = dependency('threads',static:false)
jpeg = dependency('libjpeg',static:false)
jbig = dependency('jbig2dec',static:false)
ojpeg = dependency('libopenjp2',static:false)
hb = dependency('harfbuzz',static:false)
gumbo = dependency('gumbo',static:false)
mujs = dependency('mujs',static:false)

avutil = dependency('libavutil',static:false,version:'>=57.24.100')
avcodec = dependency('libavcodec',static:false)
avdevice = dependency('libavdevice',static:false)
avformat = dependency('libavformat',static:false)
avfilter = dependency('libavfilter',static:false)

swscale = dependency('libswscale',static:false)
swresample = dependency('libswresample',static:false)
xkbcommon = dependency('xkbcommon',static:false)

wayland_client      = dependency('wayland-client')
wayland_cursor      = dependency('wayland-cursor')
wayland_protos      = dependency('wayland-protocols')
wayland_scanner_dep = dependency('wayland-scanner')
wayland_scanner     = find_program(
  wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner')
)

wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

protocols = [
  [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
  [wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml'],
  [wl_protocol_dir, 'unstable/pointer-gestures/pointer-gestures-unstable-v1.xml'],
  'wlr-layer-shell-unstable-v1.xml'
]

protos_src = []
protos_headers = []

foreach p : protocols
  xml = join_paths(p)
  protos_src += custom_target(
    xml.underscorify() + '_client_c',
    input: xml,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'public-code', '@INPUT@', '@OUTPUT@'],
  )
  protos_headers += custom_target(
    xml.underscorify() + '_client_h',
    input: xml,
    output: '@BASENAME@-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach


mmfm_dependencies = [
		  wayland_client,
		  wayland_cursor,
		  png,
		  rt,
		  freetype,
		  math,
		  gl,
		  glew,
		  sdl2,
		  avutil,
		  avcodec,
		  avdevice,
		  avformat,
		  avfilter,
		  swresample,
		  swscale,
		  pthread,
		  jpeg,
		  ojpeg,
		  jbig,
		  hb,
		  z,
		  gumbo,
		  mujs,
		  mupdf,
		  mupdfthird,
		  xkbcommon,
		  egl,
		  wegl]

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
	'src/mt_math',
	'src/mt_core',
	'src/media_player',
	'src/media_coder',
	'src/kinetic_ui',
	'src/kinetic_ui/egl',
	'src/kinetic_ui/handler',
	'src/kinetic_ui/texture',
	'src/mmfm',
	'src/mt_core_ext')


if build_machine.system() == 'freebsd'
   mmfm_inc = [
   	   mmfm_inc,
	   include_directories( 
   	   '/usr/local/include',
	   '/usr/local/include/GLES2',
	   '/usr/local/include/EGL',
	   '/usr/local/include/SDL2',
	   '/usr/local/include/mupdf',
	   '/usr/local/include/freetype2')]
   epoll = cc.find_library('epoll-shim')
   mmfm_dependencies += epoll
else
   mmfm_inc = [
   	   mmfm_inc,
	   include_directories( 
   	   '/usr/include',
	   '/usr/include/GLES2',
	   '/usr/include/EGL',
	   '/usr/include/SDL2',
	   '/usr/include/mupdf',
	   '/usr/include/freetype2')]
endif


com_sources = ['src/mmfm/ui.c',
	       'src/mmfm/config.c',
	       'src/mmfm/filemanager.c',
	       'src/mmfm/kvlist.c',
	       'src/mmfm/mmfm.c',
	       'src/mmfm/pdf.c',
	       'src/mt_core_ext/mt_string_ext.c',
	       'src/mt_core_ext/mt_map_ext.c',

	       'src/mt_core/mt_channel.c',
	       'src/mt_core/mt_string.c',
	       'src/mt_core/mt_path.c',
	       'src/mt_core/mt_log.c',
	       'src/mt_core/mt_map.c',
	       'src/mt_core/mt_memory.c',
	       'src/mt_core/mt_number.c',
	       'src/mt_core/mt_time.c',
	       'src/mt_core/mt_vector.c',
	       'src/mt_core/mt_wrapper.c',

	       'src/mt_math/mt_matrix_3d.c',
	       'src/mt_math/mt_matrix_4d.c',
	       'src/mt_math/mt_vector_2d.c',
	       'src/mt_math/mt_vector_3d.c',
	       'src/mt_math/mt_vector_4d.c',
	       'src/mt_math/mt_math_2d.c',
	       'src/mt_math/mt_math_3d.c',

	       'src/media_coder/coder.c',
	       'src/media_player/decoder.c',
	       'src/media_player/packetqueue.c',
	       'src/media_player/framequeue.c',
	       'src/media_player/clock.c',
	       'src/media_player/mediaplayer.c',

	       'src/kinetic_ui/ku_event.c',
	       'src/kinetic_ui/ku_html.c',
	       'src/kinetic_ui/ku_css.c',
	       'src/kinetic_ui/egl/ku_gl.c',
	       'src/kinetic_ui/ku_rect.c',
	       'src/kinetic_ui/ku_bitmap.c',
	       'src/kinetic_ui/texture/tg_css.c',
	       'src/kinetic_ui/texture/tg_knob.c',
	       'src/kinetic_ui/texture/tg_text.c',
	       'src/kinetic_ui/texture/tg_scaledimg.c',
	       'src/kinetic_ui/handler/vh_anim.c',
	       'src/kinetic_ui/handler/vh_button.c',
	       'src/kinetic_ui/handler/vh_drag.c',
	       'src/kinetic_ui/handler/vh_key.c',
	       'src/kinetic_ui/handler/vh_knob.c',
	       'src/kinetic_ui/handler/vh_roll.c',
	       'src/kinetic_ui/handler/vh_slider.c',
	       'src/kinetic_ui/handler/vh_textinput.c',
	       'src/kinetic_ui/handler/vh_touch.c',
	       'src/kinetic_ui/handler/vh_cv_body.c',
	       'src/kinetic_ui/handler/vh_cv_scrl.c',
	       'src/kinetic_ui/handler/vh_cv_evnt.c',
	       'src/kinetic_ui/handler/vh_tbl_body.c',
	       'src/kinetic_ui/handler/vh_tbl_evnt.c',
	       'src/kinetic_ui/handler/vh_tbl_scrl.c',
	       'src/kinetic_ui/handler/vh_tbl_head.c',
	       'src/kinetic_ui/handler/vh_table.c',
	       'src/kinetic_ui/ku_view.c',
	       'src/kinetic_ui/egl/ku_gl_atlas.c',
	       'src/kinetic_ui/egl/ku_gl_floatbuffer.c',
	       'src/kinetic_ui/egl/ku_gl_shader.c',
	       'src/kinetic_ui/ku_gen_html.c',
	       'src/kinetic_ui/ku_gen_css.c',
	       'src/kinetic_ui/ku_gen_type.c',
	       'src/kinetic_ui/ku_renderer_egl.c',
	       'src/kinetic_ui/ku_renderer_soft.c',
	       'src/kinetic_ui/ku_fontconfig.c',
	       'src/kinetic_ui/ku_recorder.c',
	       'src/kinetic_ui/ku_connector_wayland.c',
	       'src/kinetic_ui/ku_recorder.c',
	       'src/kinetic_ui/ku_window.c',
	       'src/kinetic_ui/ku_png.c',
	       'src/kinetic_ui/ku_draw.c',
	       'src/kinetic_ui/ku_text.c',
	       'src/kinetic_ui/ku_gen_textstyle.c'] + protos_src + protos_headers

pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'mmfm'
install_data('res/html/main.html', install_dir : pkg_datadir / 'html' )
install_data('res/html/main.css', install_dir : pkg_datadir / 'html' )
install_data('res/img/accept.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/close.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/closeblack.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/max.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/minus.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/next.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/pause.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/play.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/plus.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/prev.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/settings.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/sidebar.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/up.png', install_dir : pkg_datadir / 'img' )

executable(
  'mmfm',
  com_sources,
  include_directories: [mmfm_inc],
  dependencies: mmfm_dependencies,
  install: true,
)

e = executable('maintest', 'src/tests/test.c')

test('MAIN TEST', e)

# scripttests = find_program('tst/test_run.sh')

#test(
#   'SCRIPTED TESTS',
#   scripttests,
#   timeout: 0,
#   workdir : meson.current_source_dir(),
#   args : [meson.current_build_dir(),meson.current_source_dir()])
