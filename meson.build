project(
  'mmfm',
  'c',
  version: '0.53b',
  license: 'MIT',
  default_options: ['c_std=gnu99',
  		    'warning_level=3']
)

cc = meson.get_compiler('c')

# os specific libs with find library

math = cc.find_library('m')

# other deps with dependency

z = dependency('zlib')

png = dependency('libpng')
freetype = dependency('freetype2')

glesv2 = dependency('glesv2')
egl = dependency('egl')
wegl = dependency('wayland-egl')
sdl2 = dependency('SDL2')

pthread = dependency('threads')
jpeg = dependency('libjpeg')
jbig = dependency('jbig2dec')
ojpeg = dependency('libopenjp2')
hb = dependency('harfbuzz')
gumbo = dependency('gumbo')
mujs = dependency('mujs')

avutil = dependency('libavutil',version:'>=57.24.100')
avcodec = dependency('libavcodec')
avdevice = dependency('libavdevice')
avformat = dependency('libavformat')
avfilter = dependency('libavfilter')

swscale = dependency('libswscale')
swresample = dependency('libswresample')
xkbcommon = dependency('xkbcommon')

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
		  freetype,
		  math,
		  glesv2,
		  sdl2,
		  avutil,
		  avcodec,
		  avdevice,
		  avformat,
		  avfilter,
		  swresample,
		  swscale,
		  jpeg,
		  ojpeg,
		  jbig,
		  hb,
		  z,
		  gumbo,
		  mujs,
		  xkbcommon,
		  egl,
		  wegl]

if build_machine.system() == 'freebsd'
   mupdf = cc.find_library('mupdf',dirs: ['/usr/local/lib/'])
   mupdfthird = cc.find_library('mupdf-third',dirs: ['/usr/local/lib/'])
   epoll = dependency('epoll-shim')
   pthread = dependency('threads')
   mmfm_dependencies += pthread
   mmfm_dependencies += epoll
   mmfm_dependencies += mupdf
   mmfm_dependencies += mupdfthird
else
   mupdf = cc.find_library('mupdf')
   mupdfthird = cc.find_library('mupdf-third')
   mmfm_dependencies += mupdf
   mmfm_dependencies += mupdfthird
endif

mmfm_version = '"@0@"'.format(meson.project_version())
pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'mmfm'
add_project_arguments('-DPKG_DATADIR="' + pkg_datadir + '"',
		      '-DMMFM_VERSION=@0@'.format(mmfm_version),
		      '-Wno-unused-parameter',
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

com_sources = ['src/mmfm/ui.c',
	       'src/mmfm/config.c',
	       'src/mmfm/filemanager.c',
	       'src/mmfm/kvlist.c',
	       'src/mmfm/mmfm.c',
	       'src/mmfm/pdf.c',
	       'src/mt_core_ext/ku_bitmap_ext.c',
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

mt_core_test = executable(
	   'mt_core_test',
	   ['src/mt_core/mt_log.c',
	    'src/mt_core/mt_memory.c',
	    'src/mt_core/mt_vector.c',
	    'src/mt_core/mt_map.c',
	    'src/mt_core/mt_string.c',
	    'src/mt_core/mt_channel.c',
	    'src/mt_core/mt_path.c',
	    'src/mt_core_test/mt_core_test.c',
	    'src/mt_core_test/mt_memory_test.c',
	    'src/mt_core_test/mt_vector_test.c',
	    'src/mt_core_test/mt_map_test.c',
	    'src/mt_core_test/mt_string_test.c',
	    'src/mt_core_test/mt_channel_test.c',
	    'src/mt_core_test/mt_path_test.c'
	    ],
	    include_directories: ['src/mt_core'],
	    dependencies: mmfm_dependencies)

test('MT_CORE_TEST', mt_core_test)

scripttests = find_program('tst/run.sh')

test(
  'SCRIPTED TESTS',
  scripttests,
  timeout: 0,
  workdir : meson.current_source_dir(),
  args : ['BUILD_PATH=' + meson.current_build_dir(),
       	  'PROJECT_ROOT=' + meson.current_source_dir()])
