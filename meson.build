project(
  'mmfm',
  'c',
  version: '0.2',
  license: 'MIT',
  default_options: ['c_std=gnu99']
)

cc = meson.get_compiler('c')

png = cc.find_library('png',static:false)
freetype = cc.find_library('freetype',static:false)
math = cc.find_library('m',static:false)
z = cc.find_library('z',static:false)
bz2 = cc.find_library('bz2',static:false)
rt = cc.find_library('rt',static:false)
gl = cc.find_library('GL',static:false)
glew = cc.find_library('GLEW',static:false)
sdl2 = cc.find_library('SDL2',static:false)
wegl = cc.find_library('wayland-egl',static:false)
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
egl = cc.find_library('EGL',static:false)

xkbcommon = dependency('xkbcommon')

# for ffmpeg
va = cc.find_library('va',static:false)
x11 = cc.find_library('X11',static:false)
lvadrm = cc.find_library('va-drm',static:false)
lvax11 = cc.find_library('va-x11',static:false)
lzma = cc.find_library('lzma',static:false)
lvdpau = cc.find_library('vdpau',static:false)

mupdf = cc.find_library('mupdf')
mupdfthird = cc.find_library('mupdf-third')

wayland_client      = dependency('wayland-client')
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


mmfm_dependencies = [wayland_client,png,rt,freetype, math,gl,glew,sdl2,avutil,avcodec,avdevice,avformat,avfilter,swresample,swscale,pthread,jpeg,ojpeg,jbig,hb,z,bz2,x11,va,gumbo,mupdf,mupdfthird,lzma,lvadrm,lvax11,lvdpau,xkbcommon,egl,wegl]

mmfm_version = '"@0@"'.format(meson.project_version())
pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'mmfm'
add_project_arguments('-DPKG_DATADIR="' + pkg_datadir + '"',
		      '-DMMFM_VERSION=@0@'.format(mmfm_version),
#		      '-DGLEW_EGL',
		      language: 'c')
add_project_arguments('-Wno-unused-but-set-variable', language: 'c')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG',language: 'c')
endif

mmfm_inc = include_directories(
	'/usr/include',
	'/usr/include/GLES2',
	'/usr/include/EGL',
	'/usr/include/SDL2',
	'/usr/include/mupdf',
	'/usr/include/freetype2',
	'src/zen_math',
	'src/zen_core',
	'src/media_player',
	'src/media_coder',
	'src/kinetic_ui',
	'src/kinetic_ui/handler',
	'src/kinetic_ui/texture',
	'src/mmfm',
	'src/zen_core_ext')


com_sources = ['src/mmfm/ui.c',
	       'src/mmfm/config.c',
	       'src/mmfm/evrecorder.c',
	       'src/mmfm/filemanager.c',
	       'src/mmfm/kvlist.c',
	       'src/mmfm/mmfm.c',
	       'src/mmfm/pdf.c',
	       'src/zen_core_ext/zc_bitmap_ext.c',
	       'src/zen_core_ext/zc_cstr_ext.c',
	       'src/zen_core_ext/zc_map_ext.c',

	       'src/zen_core/zc_channel.c',
	       'src/zen_core/zc_cstring.c',
	       'src/zen_core/zc_path.c',
	       'src/zen_core/zc_log.c',
	       'src/zen_core/zc_map.c',
	       'src/zen_core/zc_memory.c',
	       'src/zen_core/zc_number.c',
	       'src/zen_core/zc_time.c',
	       'src/zen_core/zc_vector.c',
	       'src/zen_core/zc_wrapper.c',

	       'src/zen_math/zc_mat3.c',
	       'src/zen_math/zc_mat4.c',
	       'src/zen_math/zc_vec2.c',
	       'src/zen_math/zc_vec3.c',
	       'src/zen_math/zc_vec4.c',
	       'src/zen_math/zc_util2.c',
	       'src/zen_math/zc_util3.c',


	       'src/media_coder/coder.c',
	       'src/media_player/decoder.c',
	       'src/media_player/packetqueue.c',
	       'src/media_player/framequeue.c',
	       'src/media_player/clock.c',
	       'src/media_player/mediaplayer.c',

	       'src/kinetic_ui/ku_event.c',
	       'src/kinetic_ui/ku_html.c',
	       'src/kinetic_ui/ku_css.c',
	       'src/kinetic_ui/ku_gl.c',
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
	       'src/kinetic_ui/ku_view.c',
	       'src/kinetic_ui/ku_gl_atlas.c',
	       'src/kinetic_ui/ku_gl_floatbuffer.c',
	       'src/kinetic_ui/ku_gl_shader.c',
	       'src/kinetic_ui/ku_gen_html.c',
	       'src/kinetic_ui/ku_gen_css.c',
	       'src/kinetic_ui/ku_gen_type.c',
	       'src/kinetic_ui/ku_renderer_egl.c',
	       'src/kinetic_ui/ku_renderer_soft.c',
	       'src/kinetic_ui/ku_fontconfig.c',
	       'src/kinetic_ui/ku_connector_wayland.c',
	       'src/kinetic_ui/ku_window.c',
	       'src/kinetic_ui/ku_draw.c',
	       'src/kinetic_ui/ku_text.c',
	       'src/kinetic_ui/ku_table.c',
	       'src/kinetic_ui/ku_util.c'] + protos_src + protos_headers

pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'mmfm'
install_data('res/html/main.html', install_dir : pkg_datadir / 'html' )
install_data('res/html/main.css', install_dir : pkg_datadir / 'html' )
install_data('res/img/close.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/max.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/next.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/pause.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/play.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/prev.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/settings.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/sidebar.png', install_dir : pkg_datadir / 'img' )

executable(
  'mmfm',
  com_sources,
  include_directories: [mmfm_inc],
  dependencies: mmfm_dependencies,
  install: true,
)