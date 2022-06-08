/*
  Window Manager Connector Module for Zen Multimedia Desktop System
  Creates window and listens for events
 */

#ifndef wm_connector_h
#define wm_connector_h

#include "wm_event.c"

void wm_loop(void (*init)(int, int), void (*update)(ev_t), void (*render)(uint32_t), void (*destroy)(), char* frame);
void wm_close();
void wm_destroy();
void wm_toggle_fullscreen();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "wm_event.c"
#include "zc_log.c"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

char        wm_quit = 0;
SDL_Window* wm_window;

void wm_loop(void (*init)(int, int), void (*update)(ev_t), void (*render)(uint32_t), void (*destroy)(), char* frame)
{
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) // QUIT 0
    {
	zc_log_error("SDL could not initialize! SDL_Error: %s", SDL_GetError());
    }
    else
    {
	zc_log_debug("SDL Init Success");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_DisplayMode displaymode;
	SDL_GetCurrentDisplayMode(0, &displaymode);

	int32_t width  = displaymode.w * 0.6;
	int32_t height = displaymode.h * 0.6;

	if (frame != NULL)
	{
	    width      = atoi(frame);
	    char* next = strstr(frame, "x");
	    height     = atoi(next + 1);
	}

	wm_window = SDL_CreateWindow("Zen Music", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
				     SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE); // DESTROY 0

	if (wm_window == NULL)
	{
	    zc_log_error("SDL Window could not be created! SDL_Error: %s", SDL_GetError());
	}
	else
	{
	    zc_log_debug("SDL Window Init Success");

	    SDL_GLContext* context = SDL_GL_CreateContext(wm_window); // DELETE 0
	    if (context == NULL)
	    {
		zc_log_error("SDL Context could not be created! SDL_Error: %s", SDL_GetError());
	    }
	    else
	    {
		zc_log_debug("SDL Context Init Success");

		int nw;
		int nh;

		SDL_GL_GetDrawableSize(wm_window, &nw, &nh);

		float scale = nw / width;

		zc_log_debug("SDL Scaling will be %f", scale);

		if (SDL_GL_SetSwapInterval(1) < 0) zc_log_error("SDL swap interval error %s", SDL_GetError());

		SDL_StartTextInput(); // STOP 0

		(*init)(width, height);

		ev_t      ev = {0}; // zen event
		SDL_Event event;
		// uint32_t  lastticks = SDL_GetTicks();
		uint32_t lastclick = 0;

		while (!wm_quit)
		{
		    ev.time = SDL_GetTicks();

		    while (SDL_PollEvent(&event) != 0)
		    {
			ev.time = SDL_GetTicks();

			if (event.type == SDL_MOUSEBUTTONDOWN ||
			    event.type == SDL_MOUSEBUTTONUP ||
			    event.type == SDL_MOUSEMOTION)
			{
			    ev.ctrl_down  = SDL_GetModState() & KMOD_CTRL;
			    ev.shift_down = SDL_GetModState() & KMOD_SHIFT;

			    SDL_GetMouseState(&ev.x, &ev.y);

			    if (event.type == SDL_MOUSEBUTTONDOWN)
			    {
				ev.type   = EV_MDOWN;
				ev.button = event.button.button;
				ev.drag   = 1;
				ev.dclick = 0;

				if (lastclick == 0) lastclick = ev.time - 1000;
				uint32_t delta = ev.time - lastclick;
				lastclick      = ev.time;
				if (delta < 500) ev.dclick = 1;
				(*update)(ev);
			    }
			    else if (event.type == SDL_MOUSEBUTTONUP)
			    {
				ev.type   = EV_MUP;
				ev.button = event.button.button;
				ev.drag   = 0;
				(*update)(ev);
			    }
			    else if (event.type == SDL_MOUSEMOTION)
			    {
				ev.dx   = event.motion.xrel;
				ev.dy   = event.motion.yrel;
				ev.type = EV_MMOVE;
				(*update)(ev);
			    }
			}
			else if (event.type == SDL_MOUSEWHEEL)
			{
			    ev.type = EV_SCROLL;
			    ev.dx   = event.wheel.x * 5.0;
			    ev.dy   = event.wheel.y * 5.0;
			    (*update)(ev);
			}
			else if (event.type == SDL_QUIT)
			{
			    wm_quit = 1;
			}
			else if (event.type == SDL_WINDOWEVENT)
			{
			    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
			    {
				ev.type = EV_RESIZE;
				ev.w    = event.window.data1;
				ev.h    = event.window.data2;
				(*update)(ev);
			    }
			}
			else if (event.type == SDL_KEYDOWN)
			{
			    // printf("Scancode: 0x%02X", event.key.keysym.scancode);
			    // printf(", Name: %s\n", SDL_GetKeyName(event.key.keysym.sym));
			    ev.type    = EV_KDOWN;
			    ev.keycode = event.key.keysym.sym;
			    (*update)(ev);
			}
			else if (event.type == SDL_KEYUP)
			{
			    // printf("Scancode: 0x%02X", event.key.keysym.scancode);
			    // printf(", Name: %s\n", SDL_GetKeyName(event.key.keysym.sym));
			    ev.type    = EV_KUP;
			    ev.keycode = event.key.keysym.sym;
			    (*update)(ev);
			}
			else if (event.type == SDL_TEXTINPUT)
			{
			    ev.type = EV_TEXT;
			    strcpy(ev.text, event.text.text);
			    (*update)(ev);
			}
			else if (event.type == SDL_TEXTEDITING)
			{
			    ev.type = EV_TEXT;
			    strcpy(ev.text, event.text.text);
			    (*update)(ev);
			}
			else
			    zc_log_debug("unknown event, sdl type %i", event.type);
		    }

		    ev.type = EV_TIME;

		    (*update)(ev);
		    (*render)(ev.time);
		    SDL_GL_SwapWindow(wm_window);
		}

		(*destroy)();

		SDL_StopTextInput();           // STOP 0
		SDL_GL_DeleteContext(context); // DELETE 0

		zc_log_debug("SDL Context deleted");
	    }
	}

	SDL_DestroyWindow(wm_window); // DESTROY 0
	SDL_Quit();                   // QUIT 0
	zc_log_debug("SDL Window destroyed");
    }
}

void wm_close()
{
    wm_quit = 1;
}

void wm_toggle_fullscreen()
{
    int flags = SDL_GetWindowFlags(wm_window);

    char fullscreen = SDL_GetWindowFlags(wm_window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    flags ^= SDL_WINDOW_FULLSCREEN_DESKTOP;

    if (fullscreen == 1)
	SDL_SetWindowFullscreen(wm_window, flags);
    else
	SDL_SetWindowFullscreen(wm_window, flags | SDL_WINDOW_FULLSCREEN_DESKTOP);
}

#endif
