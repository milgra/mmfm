#ifndef ku_wayland_h
#define ku_wayland_h

#include "ku_bitmap.c"
#include "ku_event.c"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <wayland-egl.h>
#include <xkbcommon/xkbcommon.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "zc_log.c"
#include "zc_memory.c"
#include "zc_time.c"

#define MAX_MONITOR_NAME_LEN 255

enum wl_event_id_t
{
    WL_EVENT_OUTPUT_ADDED,
    WL_EVENT_OUTPUT_REMOVED,
    WL_EVENT_OUTPUT_CHANGED
};

struct monitor_info
{
    int32_t physical_width;
    int32_t physical_height;
    int32_t logical_width;
    int32_t logical_height;
    int     scale;
    double  ratio;
    int     index;
    char    name[MAX_MONITOR_NAME_LEN + 1];

    enum wl_output_subpixel subpixel;
    struct zxdg_output_v1*  xdg_output;
    struct wl_output*       wl_output;
};

enum wl_window_type
{
    WL_WINDOW_NATIVE,
    WL_WINDOW_EGL
};

struct wl_window
{
    enum wl_window_type type;

    struct xdg_surface*  xdg_surface;  // window surface
    struct xdg_toplevel* xdg_toplevel; // window info
    struct wl_surface*   surface;      // wl surface for window
    struct wl_buffer*    buffer;       // wl buffer for surface
    void*                shm_data;     // active bufferdata
    int                  shm_size;     // active bufferdata size
    ku_bitmap_t          bitmap;

    int scale;
    int width;
    int height;
    int new_scale;
    int new_width;
    int new_height;

    struct wl_callback* frame_cb;

    struct monitor_info* monitor;

    struct wl_egl_window* eglwindow;

    struct wl_region* region;

    EGLDisplay egldisplay;
    EGLSurface eglsurface;
};

struct layer_info
{
    struct zwlr_layer_shell_v1*   layer_shell;   // active layer shell
    struct zwlr_layer_surface_v1* layer_surface; // active layer surface

    struct wl_surface* surface;  // wl surface for window
    struct wl_buffer*  buffer;   // wl buffer for surface
    void*              shm_data; // active bufferdata
    int                shm_size; // active bufferdata size
    ku_bitmap_t        bitmap;
};

typedef struct _wl_event_t wl_event_t;
struct _wl_event_t
{
    enum wl_event_id_t    id;
    struct monitor_info** monitors;
    int                   monitor_count;
};

void ku_wayland_init(
    void (*init)(wl_event_t event),
    void (*event)(wl_event_t event),
    void (*update)(ku_event_t),
    void (*render)(uint32_t time, uint32_t index, ku_bitmap_t* bm),
    void (*destroy)());

void ku_wayland_draw();
void wl_hide();

struct wl_window* ku_wayland_create_window(char* title, int width, int height);
void              ku_wayland_draw_window(struct wl_window* info, int x, int y, int w, int h);
void              ku_wayland_delete_window(struct wl_window* info);

struct wl_window* ku_wayland_create_eglwindow(char* title, int width, int height);

void ku_wayland_create_layer();
void ku_wayland_delete_layer();

void ku_wayland_exit();
void ku_wayland_toggle_fullscreen();

#endif

#if __INCLUDE_LEVEL__ == 0

struct keyboard_info
{
    struct wl_keyboard* kbd;
    struct xkb_context* xkb_context;
    struct xkb_keymap*  xkb_keymap;
    struct xkb_state*   xkb_state;
    bool                control;
    bool                shift;
};
struct wlc_t
{
    struct wl_display* display; // global display object

    struct wl_compositor* compositor; // active compositor
    struct wl_pointer*    pointer;    // active pointer for seat
    struct wl_seat*       seat;       // active seat

    struct wl_shm* shm; // active shared memory buffer

    struct zxdg_output_manager_v1* xdg_output_manager; // active xdg output manager
    struct zwlr_layer_shell_v1*    layer_shell;        // active layer shell
    struct zwlr_layer_surface_v1*  layer_surface;      // active layer surface

    // outputs

    int                   monitor_count;
    struct monitor_info** monitors;
    struct monitor_info*  monitor;
    struct keyboard_info  keyboard;

    struct wl_window** windows;

    // window state

    void (*init)(wl_event_t event);
    void (*event)(wl_event_t event);
    void (*update)(ku_event_t);
    void (*render)(uint32_t time, uint32_t index, ku_bitmap_t* bm);
    void (*destroy)();

    struct xdg_wm_base* xdg_wm_base;

} wlc = {0};

int ku_wayland_exit_flag = 0;

void ku_wayland_create_buffer();
void ku_wayland_resize_window_buffer(struct wl_window* info);

int32_t round_to_int(double val)
{
    return (int32_t) (val + 0.5);
}

int ku_wayland_shm_create()
{
    int  shmid = -1;
    char shm_name[NAME_MAX];
    for (int i = 0; i < UCHAR_MAX; ++i)
    {
	if (snprintf(shm_name, NAME_MAX, "/wcp-%d", i) >= NAME_MAX)
	{
	    break;
	}
	shmid = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (shmid > 0 || errno != EEXIST)
	{
	    break;
	}
    }

    if (shmid < 0)
    {
	zc_log_debug("shm_open() failed: %s", strerror(errno));
	return -1;
    }

    if (shm_unlink(shm_name) != 0)
    {
	zc_log_debug("shm_unlink() failed: %s", strerror(errno));
	return -1;
    }

    return shmid;
}

void* ku_wayland_shm_alloc(const int shmid, const size_t size)
{
    if (ftruncate(shmid, size) != 0)
    {
	zc_log_debug("ftruncate() failed: %s", strerror(errno));
	return NULL;
    }

    void* buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    if (buffer == MAP_FAILED)
    {
	zc_log_debug("mmap() failed: %s", strerror(errno));
	return NULL;
    }

    return buffer;
}

static void ku_wayland_buffer_release(void* data, struct wl_buffer* wl_buffer)
{
    /* zc_log_debug("buffer release"); */
}

static const struct wl_buffer_listener buffer_listener = {
    .release = ku_wayland_buffer_release};

void ku_wayland_create_buffer(struct wl_window* info, int width, int height)
{
    zc_log_debug("create buffer %i %i", width, height);

    if (info->buffer)
    {
	// delete old buffer and bitmap
	wl_buffer_destroy(info->buffer);
	munmap(info->shm_data, info->shm_size);
	info->bitmap.data = NULL;
    }

    int stride = width * 4;
    int size   = stride * height;

    int fd = ku_wayland_shm_create();
    if (fd < 0)
    {
	zc_log_error("creating a buffer file for %d B failed: %m", size);
	return;
    }

    info->shm_data = ku_wayland_shm_alloc(fd, size);

    info->bitmap.w    = width;
    info->bitmap.h    = height;
    info->bitmap.size = size;
    info->bitmap.data = info->shm_data;

    if (info->shm_data == MAP_FAILED)
    {
	zc_log_error("mmap failed: %m");
	close(fd);
	return;
    }

    struct wl_shm_pool* pool   = wl_shm_create_pool(wlc.shm, fd, size);
    struct wl_buffer*   buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(pool);

    info->buffer   = buffer;
    info->shm_size = size;

    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
    zc_log_debug("buffer listener added");
}

/* frame listener */

static const struct wl_callback_listener wl_surface_frame_listener;

static uint32_t lasttime  = 0;
static uint32_t lastcount = 0;

static void wl_surface_frame_done(void* data, struct wl_callback* cb, uint32_t time)
{
    /* zc_log_debug("*************************wl surface frame done %u", time); */

    struct wl_window* info = data;
    /* Destroy this callback */
    wl_callback_destroy(cb);

    info->frame_cb = NULL;

    if (time - lasttime > 1000)
    {
	/* printf("FRAME PER SEC : %u\n", lastcount); */
	lastcount = 0;
	lasttime  = time;
    }

    lastcount++;

    ku_event_t event = {
	.type = KU_EVENT_TIME,
	.time = time};

    // TODO refresh rate can differ so animations should be time based
    (*wlc.update)(event);

    /* info->frame_cb = wl_surface_frame(info->surface); */
    /* wl_callback_add_listener(info->frame_cb, &wl_surface_frame_listener, info); */

    /* glClearColor(0.5, 0.3, 0.0, 1.0); */
    /* glClear(GL_COLOR_BUFFER_BIT); */

    /* eglSwapBuffers(wlc.windows[0]->egldisplay, wlc.windows[0]->eglsurface); */

    /* ku_wayland_draw(); */
    /* Submit a frame for this event */

    /* wl_surface_attach(wlc.surface, buffer, 0, 0); */
    /* wl_surface_damage_buffer(wlc.surface, 0, 0, INT32_MAX, INT32_MAX); */
    /* wl_surface_commit(wlc.surface); */
}

static const struct wl_callback_listener wl_surface_frame_listener = {
    .done = wl_surface_frame_done,
};

/* layer surface listener */

static void ku_wayland_layer_surface_configure(void* data, struct zwlr_layer_surface_v1* surface, uint32_t serial, uint32_t width, uint32_t height)
{
    zc_log_debug("layer surface configure serial %u width %i height %i", serial, width, height);

    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void ku_wayland_layer_surface_closed(void* _data, struct zwlr_layer_surface_v1* surface)
{
    zc_log_debug("layer surface configure");
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = ku_wayland_layer_surface_configure,
    .closed    = ku_wayland_layer_surface_closed,
};

/* xdg toplevel events */

void xdg_toplevel_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states)
{
    zc_log_debug("xdg toplevel configure w %i h %i", width, height);

    struct wl_window* info = data;

    if (width > 0 && height > 0)
    {
	info->new_width  = width;
	info->new_height = height;
    }
}

void xdg_toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel)
{
    zc_log_debug("xdg toplevel close");
}

void xdg_toplevel_configure_bounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height)
{
    zc_log_debug("xdg toplevel configure bounds w %i h %i", width, height);
}

void xdg_toplevel_wm_capabilities(void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities)
{
    zc_log_debug("xdg toplevel wm capabilities");
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure        = xdg_toplevel_configure,
    .close            = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities  = xdg_toplevel_wm_capabilities};

/* xdg surface events */

static void xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
    zc_log_debug("xdg surface configure");

    struct wl_window* info = data;

    xdg_surface_ack_configure(xdg_surface, serial);

    if (info->type == WL_WINDOW_NATIVE) ku_wayland_resize_window_buffer(info);
    else
    {
	if (info->width != info->new_width && info->height != info->new_height)
	{
	    info->width  = info->new_width;
	    info->height = info->new_height;

	    printf("RESIZE EGLWINDOW %i %i\n", info->width, info->height);

	    wl_egl_window_resize(info->eglwindow, info->width, info->height, 0, 0);
	    /* wl_surface_commit(info->surface); */

	    /* wl_display_dispatch_pending(wlc.display); */

	    /* This space deliberately left blank */

	    glClearColor(0.5, 0.3, 0.0, 1.0);
	    glClear(GL_COLOR_BUFFER_BIT);

	    eglSwapBuffers(wlc.windows[0]->egldisplay, wlc.windows[0]->eglsurface);
	}
    }

    /* ku_wayland_resize_window_buffer(info); */
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

/* wl surface events */

static void wl_surface_enter(void* userData, struct wl_surface* surface, struct wl_output* output)
{
    zc_log_debug("wl surface enter");
    struct wl_window* info = userData;

    if (info->type == WL_WINDOW_NATIVE)
    {

	for (int index = 0; index < wlc.monitor_count; index++)
	{
	    struct monitor_info* monitor = wlc.monitors[index];

	    if (monitor->wl_output == output)
	    {
		zc_log_debug("output name %s %i %i", monitor->name, monitor->scale, info->scale);

		if (monitor->scale != info->scale)
		{
		    info->monitor   = monitor;
		    info->new_scale = monitor->scale;
		    ku_wayland_resize_window_buffer(info);
		}
	    }
	}
    }
}

static void wl_surface_leave(void* userData, struct wl_surface* surface, struct wl_output* output)
{
    struct wl_window* info = userData;
    zc_log_debug("wl surface leave");
}

static const struct wl_surface_listener wl_surface_listener = {
    .enter = wl_surface_enter,
    .leave = wl_surface_leave,
};

/* creates xdg surface and toplevel */

struct wl_window* ku_wayland_create_window(char* title, int width, int height)
{
    struct wl_window* info = CAL(sizeof(struct wl_window), NULL, NULL);

    info->type = WL_WINDOW_NATIVE;

    wlc.windows[0] = info;

    info->new_scale  = 1;
    info->new_width  = width;
    info->new_height = height;

    info->surface = wl_compositor_create_surface(wlc.compositor);
    wl_surface_add_listener(info->surface, &wl_surface_listener, info);

    info->xdg_surface = xdg_wm_base_get_xdg_surface(wlc.xdg_wm_base, info->surface);
    xdg_surface_add_listener(info->xdg_surface, &xdg_surface_listener, info);

    info->xdg_toplevel = xdg_surface_get_toplevel(info->xdg_surface);
    xdg_toplevel_add_listener(info->xdg_toplevel, &xdg_toplevel_listener, info);

    xdg_toplevel_set_title(info->xdg_toplevel, title);

    /* wl_display_roundtrip(wlc.display); */

    wl_surface_commit(info->surface);

    info->frame_cb = wl_surface_frame(info->surface);
    wl_callback_add_listener(info->frame_cb, &wl_surface_frame_listener, info);

    return info;
}

struct wl_window* ku_wayland_create_eglwindow(char* title, int width, int height)
{
    struct wl_window* info = CAL(sizeof(struct wl_window), NULL, NULL);

    info->type = WL_WINDOW_EGL;

    wlc.windows[0] = info;

    info->new_scale  = 1;
    info->new_width  = width;
    info->new_height = height;

    info->surface = wl_compositor_create_surface(wlc.compositor);
    wl_surface_add_listener(info->surface, &wl_surface_listener, info);

    info->xdg_surface = xdg_wm_base_get_xdg_surface(wlc.xdg_wm_base, info->surface);
    xdg_surface_add_listener(info->xdg_surface, &xdg_surface_listener, info);

    info->xdg_toplevel = xdg_surface_get_toplevel(info->xdg_surface);
    xdg_toplevel_add_listener(info->xdg_toplevel, &xdg_toplevel_listener, info);
    xdg_toplevel_set_title(info->xdg_toplevel, title);

    wl_surface_commit(info->surface);

    info->frame_cb = wl_surface_frame(info->surface);
    wl_callback_add_listener(info->frame_cb, &wl_surface_frame_listener, info);

    info->region = wl_compositor_create_region(wlc.compositor);

    wl_region_add(info->region, 0, 0, width, height);
    wl_surface_set_opaque_region(info->surface, info->region);

    struct wl_egl_window* egl_window = wl_egl_window_create(info->surface, width, height);

    info->eglwindow = egl_window;

    if (egl_window == EGL_NO_SURFACE)
    {
	printf("No window !?\n");
    }
    else printf("Window created !\n");

    EGLint     numConfigs;
    EGLint     majorVersion;
    EGLint     minorVersion;
    EGLContext context;
    EGLSurface surface;
    EGLConfig  config;
    EGLint     fbAttribs[] =
	{
	    EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE};

    EGLDisplay display = eglGetDisplay(wlc.display);
    if (display == EGL_NO_DISPLAY)
    {
	printf("No EGL Display...\n");
    }

    info->egldisplay = display;

    // Initialize EGL
    if (!eglInitialize(display, &majorVersion, &minorVersion))
    {
	printf("No Initialisation...\n");
    }

    // Get configs
    if ((eglGetConfigs(display, NULL, 0, &numConfigs) != EGL_TRUE) || (numConfigs == 0))
    {
	printf("No configuration...\n");
    }

    // Choose config
    if ((eglChooseConfig(display, fbAttribs, &config, 1, &numConfigs) != EGL_TRUE) || (numConfigs != 1))
    {
	printf("No configuration...\n");
    }

    // Create a surface
    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType) egl_window, NULL);
    if (surface == EGL_NO_SURFACE)
    {
	printf("No surface...\n");
    }

    info->eglsurface = surface;

    // Create a GL context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT)
    {
	printf("No context...\n");
    }

    // Make the context current
    if (!eglMakeCurrent(display, surface, surface, context))
    {
	printf("Could not make the current window current !\n");
    }

    printf("dispatch pending\n");

    wl_display_dispatch_pending(wlc.display);

    /* This space deliberately left blank */

    /* glClearColor(0.5, 0.3, 0.0, 1.0); */
    /* glClear(GL_COLOR_BUFFER_BIT); */

    /* eglSwapBuffers(wlc.windows[0]->egldisplay, wlc.windows[0]->eglsurface); */

    /* wl_surface_commit(info->surface); */

    /* while (1) */
    /* { */
    /* 	wl_display_dispatch_pending(wlc.display); */

    /* 	glClearColor(0.5, 0.3, 0.0, 1.0); */
    /* 	glClear(GL_COLOR_BUFFER_BIT); */

    /* 	eglSwapBuffers(display, surface); */
    /* } */

    return info;
}

/* deletes xdg surface and toplevel */

void ku_wayland_delete_window(struct wl_window* info)
{
    xdg_surface_destroy(info->xdg_surface);
    wl_surface_destroy(info->surface);
    xdg_toplevel_destroy(info->xdg_toplevel);

    wl_display_roundtrip(wlc.display);
}

/* resizes buffer on surface configure */

void ku_wayland_resize_window_buffer(struct wl_window* info)
{
    if (info->new_width != info->width || info->new_height != info->height || info->new_scale != info->scale)
    {
	zc_log_debug("BUFFER RESIZE w %i h %i s %i", info->new_width, info->new_height, info->new_scale);

	int32_t nwidth  = round_to_int(info->new_width * info->new_scale);
	int32_t nheight = round_to_int(info->new_height * info->new_scale);

	info->scale  = info->new_scale;
	info->width  = nwidth;
	info->height = nheight;

	ku_wayland_create_buffer(info, nwidth, nheight);

	wl_surface_attach(info->surface, info->buffer, 0, 0);
	wl_surface_commit(info->surface);

	ku_event_t event = {
	    .type = KU_EVENT_RESIZE,
	    .w    = nwidth,
	    .h    = nheight};

	(*wlc.update)(event);
    }
}

/* update surface with bitmap data */

void ku_wayland_draw_window(struct wl_window* info, int x, int y, int w, int h)
{
    /* Request another frame */
    if (info->frame_cb == NULL)
    {
	info->frame_cb = wl_surface_frame(info->surface);
	wl_callback_add_listener(info->frame_cb, &wl_surface_frame_listener, info);

	if (info->type == WL_WINDOW_NATIVE)
	{
	    wl_surface_attach(info->surface, info->buffer, 0, 0);
	    wl_surface_damage(info->surface, x, y, w, h);
	    wl_surface_commit(info->surface);
	}
	else if (info->type == WL_WINDOW_EGL)
	{
	    glClearColor(0.5, 0.3, 0.0, 1.0);
	    glClear(GL_COLOR_BUFFER_BIT);

	    eglSwapBuffers(wlc.windows[0]->egldisplay, wlc.windows[0]->eglsurface);
	}
    }
    else printf("NO DRAW\n");
}

/* pointer listener */

// TODO differentiate these by wl_pointer address
int px   = 0;
int py   = 0;
int drag = 0;

void ku_wayland_pointer_handle_enter(void* data, struct wl_pointer* wl_pointer, uint serial, struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    zc_log_debug("pointer handle enter");
}
void ku_wayland_pointer_handle_leave(void* data, struct wl_pointer* wl_pointer, uint serial, struct wl_surface* surface)
{
    zc_log_debug("pointer handle leave");
}
void ku_wayland_pointer_handle_motion(void* data, struct wl_pointer* wl_pointer, uint time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    /* zc_log_debug("pointer handle motion %f %f", wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y)); */

    ku_event_t event = {
	.type = KU_EVENT_MMOVE,
	.drag = drag,
	.x    = (int) wl_fixed_to_double(surface_x) * wlc.monitor->scale,
	.y    = (int) wl_fixed_to_double(surface_y) * wlc.monitor->scale};

    px = event.x;
    py = event.y;

    (*wlc.update)(event);
}

uint lasttouch = 0;
void ku_wayland_pointer_handle_button(void* data, struct wl_pointer* wl_pointer, uint serial, uint time, uint button, uint state)
{
    zc_log_debug("pointer handle button %u state %u time %u", button, state, time);

    ku_event_t event = {.x = px, .y = py, .button = button == 272 ? 1 : 3};

    if (state)
    {
	uint delay = time - lasttouch;
	lasttouch  = time;

	event.dclick = delay < 300;
	event.type   = KU_EVENT_MDOWN;
	drag         = 1;
    }
    else
    {
	event.type = KU_EVENT_MUP;
	drag       = 0;
    }

    (*wlc.update)(event);
}
void ku_wayland_pointer_handle_axis(void* data, struct wl_pointer* wl_pointer, uint time, uint axis, wl_fixed_t value)
{
    /* zc_log_debug("pointer handle axis %u %i", axis, value); */
    ku_event_t event =
	{.type       = KU_EVENT_SCROLL,
	 .x          = px,
	 .y          = py,
	 .dx         = axis == 1 ? (float) value / 200.0 : 0,
	 .dy         = axis == 0 ? (float) -value / 200.0 : 0,
	 .ctrl_down  = wlc.keyboard.control,
	 .shift_down = wlc.keyboard.shift};

    (*wlc.update)(event);
}
void ku_wayland_pointer_handle_frame(void* data, struct wl_pointer* wl_pointer)
{
    zc_log_debug("pointer handle frame");
}
void ku_wayland_pointer_handle_axis_source(void* data, struct wl_pointer* wl_pointer, uint axis_source)
{
    zc_log_debug("pointer handle axis source");
}
void ku_wayland_pointer_handle_axis_stop(void* data, struct wl_pointer* wl_pointer, uint time, uint axis)
{
    zc_log_debug("pointer handle axis stop");
}
void ku_wayland_pointer_handle_axis_discrete(void* data, struct wl_pointer* wl_pointer, uint axis, int discrete)
{
    zc_log_debug("pointer handle axis discrete");
}

struct wl_pointer_listener pointer_listener =
    {
	.enter         = ku_wayland_pointer_handle_enter,
	.leave         = ku_wayland_pointer_handle_leave,
	.motion        = ku_wayland_pointer_handle_motion,
	.button        = ku_wayland_pointer_handle_button,
	.axis          = ku_wayland_pointer_handle_axis,
	.frame         = ku_wayland_pointer_handle_frame,
	.axis_source   = ku_wayland_pointer_handle_axis_source,
	.axis_stop     = ku_wayland_pointer_handle_axis_stop,
	.axis_discrete = ku_wayland_pointer_handle_axis_discrete,
};

/* lkeyboard listener */

static void keyboard_keymap(void* data, struct wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
{
    zc_log_debug("keyboard keymap");

    wlc.keyboard.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
	close(fd);
	exit(1);
    }
    char* map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_shm == MAP_FAILED)
    {
	close(fd);
	exit(1);
    }
    wlc.keyboard.xkb_keymap = xkb_keymap_new_from_string(wlc.keyboard.xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    munmap(map_shm, size);
    close(fd);

    wlc.keyboard.xkb_state = xkb_state_new(wlc.keyboard.xkb_keymap);
}

static void keyboard_enter(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{
    zc_log_debug("keyboard enter");
}

static void keyboard_leave(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface)
{
    zc_log_debug("keyboard leave");
}

static void keyboard_key(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t _key_state)
{
    enum wl_keyboard_key_state key_state = _key_state;
    xkb_keysym_t               sym       = xkb_state_key_get_one_sym(wlc.keyboard.xkb_state, key + 8);

    /* switch (xkb_keysym_to_lower(sym)) */

    if (!(sym == XKB_KEY_BackSpace ||
	  sym == XKB_KEY_Escape ||
	  sym == XKB_KEY_Return ||
	  sym == XKB_KEY_Print))
    {
	char buf[8];
	if (xkb_keysym_to_utf8(sym, buf, 8))
	{
	    if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED)
	    {
		ku_event_t event = {
		    .keycode    = sym,
		    .type       = KU_EVENT_TEXT,
		    .time       = time,
		    .ctrl_down  = wlc.keyboard.control,
		    .shift_down = wlc.keyboard.shift};

		memcpy(event.text, buf, 8);
		(*wlc.update)(event);
	    }
	}
    }

    ku_event_t event = {
	.keycode    = sym,
	.type       = key_state == WL_KEYBOARD_KEY_STATE_PRESSED ? KU_EVENT_KDOWN : KU_EVENT_KUP,
	.time       = time,
	.ctrl_down  = wlc.keyboard.control,
	.shift_down = wlc.keyboard.shift};
    (*wlc.update)(event);

    zc_log_debug("keyboard key %i", key);
}

static void keyboard_repeat_info(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay)
{
    zc_log_debug("keyboard repeat info %i %i", rate, delay);
    /*    panel->repeat_delay = delay; */
    /*     if (rate > 0) */
    /* 	panel->repeat_period = 1000 / rate; */
    /*     else */
    /* 	panel->repeat_period = -1; */
}

static void keyboard_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    zc_log_debug("keyboard modifiers");
    xkb_state_update_mask(wlc.keyboard.xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    wlc.keyboard.control = xkb_state_mod_name_is_active(wlc.keyboard.xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
    wlc.keyboard.shift   = xkb_state_mod_name_is_active(wlc.keyboard.xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap      = keyboard_keymap,
    .enter       = keyboard_enter,
    .leave       = keyboard_leave,
    .key         = keyboard_key,
    .modifiers   = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info,
};

/* wm base listener */

static void xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

/* xdg output events */

static void ku_wayland_xdg_output_handle_logical_position(void* data, struct zxdg_output_v1* xdg_output, int32_t x, int32_t y)
{
    struct monitor_info* monitor = data;
    zc_log_debug("xdg output handle logical position, %i %i for monitor %i", x, y, monitor->index);
}

static void ku_wayland_xdg_output_handle_logical_size(void* data, struct zxdg_output_v1* xdg_output, int32_t width, int32_t height)
{
    struct monitor_info* monitor = data;
    zc_log_debug("xdg output handle logical size, %i %i for monitor %i", width, height, monitor->index);

    monitor->logical_width  = width;
    monitor->logical_height = height;
}

static void ku_wayland_xdg_output_handle_done(void* data, struct zxdg_output_v1* xdg_output)
{
    struct monitor_info* monitor = data;
    zc_log_debug("xdg output handle done, for monitor %i", monitor->index);
}

static void ku_wayland_xdg_output_handle_name(void* data, struct zxdg_output_v1* xdg_output, const char* name)
{
    struct monitor_info* monitor = data;
    strncpy(monitor->name, name, MAX_MONITOR_NAME_LEN);

    zc_log_debug("xdg output handle name, %s for monitor %i", name, monitor->index);
}

static void ku_wayland_xdg_output_handle_description(void* data, struct zxdg_output_v1* xdg_output, const char* description)
{
    struct monitor_info* monitor = data;
    zc_log_debug("xdg output handle description for monitor %i", description, monitor->index);
}

struct zxdg_output_v1_listener xdg_output_listener = {
    .logical_position = ku_wayland_xdg_output_handle_logical_position,
    .logical_size     = ku_wayland_xdg_output_handle_logical_size,
    .done             = ku_wayland_xdg_output_handle_done,
    .name             = ku_wayland_xdg_output_handle_name,
    .description      = ku_wayland_xdg_output_handle_description,
};

/* output events */

static void ku_wayland_wl_output_handle_geometry(
    void*             data,
    struct wl_output* wl_output,
    int32_t           x,
    int32_t           y,
    int32_t           width_mm,
    int32_t           height_mm,
    int32_t           subpixel,
    const char*       make,
    const char*       model,
    int32_t           transform)
{
    struct monitor_info* monitor = data;

    zc_log_debug(
	"wl output handle geometry x %i y %i width_mm %i height_mm %i subpixel %i make %s model %s transform %i for monitor %i",
	x,
	y,
	width_mm,
	height_mm,
	subpixel,
	make,
	model,
	transform,
	monitor->index);

    monitor->subpixel = subpixel;
}

static void ku_wayland_wl_output_handle_mode(
    void*             data,
    struct wl_output* wl_output,
    uint32_t          flags,
    int32_t           width,
    int32_t           height,
    int32_t           refresh)
{
    struct monitor_info* monitor = data;

    zc_log_debug(
	"wl output handle mode flags %u width %i height %i for monitor %i",
	flags,
	width,
	height,
	monitor->index);

    monitor->physical_width  = width;
    monitor->physical_height = height;
}

static void ku_wayland_wl_output_handle_done(void* data, struct wl_output* wl_output)
{
    struct monitor_info* monitor = data;

    zc_log_debug("wl output handle done for monitor %i", monitor->index);
}

static void ku_wayland_wl_output_handle_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
    struct monitor_info* monitor = data;

    zc_log_debug("wl output handle scale %i for monitor %i", factor, monitor->index);

    monitor->scale = factor;
}

struct wl_output_listener wl_output_listener = {
    .geometry = ku_wayland_wl_output_handle_geometry,
    .mode     = ku_wayland_wl_output_handle_mode,
    .done     = ku_wayland_wl_output_handle_done,
    .scale    = ku_wayland_wl_output_handle_scale,
};

/* seat events */

static void ku_wayland_seat_handle_capabilities(void* data, struct wl_seat* wl_seat, enum wl_seat_capability caps)
{
    zc_log_debug("seat handle capabilities %i", caps);
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
	wlc.keyboard.kbd = wl_seat_get_keyboard(wl_seat);
	wl_keyboard_add_listener(wlc.keyboard.kbd, &keyboard_listener, NULL);
    }

    if (caps & WL_SEAT_CAPABILITY_POINTER)
    {
	wlc.pointer = wl_seat_get_pointer(wl_seat);
	wl_pointer_add_listener(wlc.pointer, &pointer_listener, NULL);
    }
}

static void ku_wayland_seat_handle_name(void* data, struct wl_seat* wl_seat, const char* name)
{
    zc_log_debug("seat handle name %s", name);
}

const struct wl_seat_listener seat_listener = {
    .capabilities = ku_wayland_seat_handle_capabilities,
    .name         = ku_wayland_seat_handle_name,
};

/* global events */

static void ku_wayland_handle_global(
    void*               data,
    struct wl_registry* registry,
    uint32_t            name,
    const char*         interface,
    uint32_t            version)
{
    zc_log_debug("handle global : %s, version %u", interface, version);

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
	wlc.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
	wlc.seat = wl_registry_bind(registry, name, &wl_seat_interface, 4);
	wl_seat_add_listener(wlc.seat, &seat_listener, NULL);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
	wlc.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
	if (wlc.monitor_count >= 16) return;

	struct monitor_info* monitor = malloc(sizeof(struct monitor_info));
	memset(monitor->name, 0, MAX_MONITOR_NAME_LEN);
	monitor->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 2);
	monitor->index     = wlc.monitor_count;

	// get wl_output events
	wl_output_add_listener(monitor->wl_output, &wl_output_listener, monitor);

	// set up output if it comes after xdg_output_manager_init
	if (wlc.xdg_output_manager != NULL)
	{
	    monitor->xdg_output = zxdg_output_manager_v1_get_xdg_output(wlc.xdg_output_manager, monitor->wl_output);
	    zxdg_output_v1_add_listener(monitor->xdg_output, &xdg_output_listener, monitor);
	}

	wlc.monitors[wlc.monitor_count++] = monitor;
    }
    else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
	wlc.layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    }
    else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0)
    {
	wlc.xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);

	// set up outputs if event comes after interface setup
	for (int index = 0; index < wlc.monitor_count; index++)
	{
	    wlc.monitors[index]->xdg_output = zxdg_output_manager_v1_get_xdg_output(wlc.xdg_output_manager, wlc.monitors[index]->wl_output);
	    zxdg_output_v1_add_listener(wlc.monitors[index]->xdg_output, &xdg_output_listener, wlc.monitors[index]);
	}
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
	wlc.xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	xdg_wm_base_add_listener(wlc.xdg_wm_base, &xdg_wm_base_listener, NULL);
    }
}

static void ku_wayland_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
    zc_log_debug("handle global remove");
}

static const struct wl_registry_listener registry_listener =
    {.global        = ku_wayland_handle_global,
     .global_remove = ku_wayland_handle_global_remove};

/* init wayland */

void ku_wayland_init(
    void (*init)(wl_event_t event),
    void (*event)(wl_event_t event),
    void (*update)(ku_event_t event),
    void (*render)(uint32_t time, uint32_t index, ku_bitmap_t* bm),
    void (*destroy)())
{
    wlc.monitors = CAL(sizeof(struct monitor_info) * 16, NULL, NULL);
    wlc.windows  = CAL(sizeof(struct wl_window) * 16, NULL, NULL);

    wlc.init    = init;
    wlc.event   = event;
    wlc.render  = render;
    wlc.update  = update;
    wlc.destroy = destroy;

    wlc.display = wl_display_connect(NULL);

    if (wlc.display)
    {
	struct wl_registry* registry = wl_display_get_registry(wlc.display);
	wl_registry_add_listener(registry, &registry_listener, NULL);

	// first roundtrip triggers global events
	wl_display_roundtrip(wlc.display);

	// second roundtrip triggers events attached in global events
	wl_display_roundtrip(wlc.display);
	if (wlc.compositor)
	{
	    struct _wl_event_t event = {.id = WL_EVENT_OUTPUT_ADDED, .monitors = wlc.monitors, .monitor_count = wlc.monitor_count};

	    (*wlc.init)(event);

	    wlc.monitor = wlc.monitors[0];

	    while (wl_display_dispatch(wlc.display) > -1)
	    {
		printf("DISSP\n");
		/* This space deliberately left blank */
		if (ku_wayland_exit_flag) break;
	    }

	    wl_display_disconnect(wlc.display);
	}
	else zc_log_debug("compositor not received");
    }
    else zc_log_debug("cannot open display");

    (*wlc.destroy)();
}

void ku_wayland_exit()
{
    ku_wayland_exit_flag = 1;
}
void ku_wayland_toggle_fullscreen(struct wl_window* window)
{
    xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);
}

#endif
