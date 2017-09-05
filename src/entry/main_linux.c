#ifdef BR_PLATFORM_LINUX

#include "entry.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <time.h> // nanosleep
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "log.h"

// https://tronche.com/gui/x/xlib/events/processing-overview.html

// TODO: Check for errors.

static void sleep(uint32_t msec) {
	struct timespec req = { (time_t)msec/1000, (long)((msec % 1000) * 1000000) };
	struct timespec rem = { 0, 0 };
	nanosleep(&req, &rem);
}

static entry_window_info_t s_window;

const entry_window_info_t* entry_get_window() {
	return &s_window;
}

int main(int argc, const char* argv[]) {
	XInitThreads();

	Display* display = XOpenDisplay(NULL);
	const int screen = DefaultScreen(display);

	const uint16_t w = 600;
	const uint16_t h = 400;

	Window window = XCreateSimpleWindow(
		display, DefaultRootWindow(display),
		0, 0, w, h,
		0, WhitePixel(display, screen), BlackPixel(display, screen));

	s_window.display = display;
	s_window.window = (void*)(uintptr_t)window;
	s_window.width = w;
	s_window.height = h;

	if (!entry_init(argc, argv))
		// TODO: cleanup.
		return 1;

	XSelectInput(display, window, StructureNotifyMask | ButtonPressMask | KeyPressMask);
	XStoreName(display, window, "b4re");
	XClassHint* hint = XAllocClassHint();
	hint->res_name = "b4re";
	hint->res_class = "b4re_class";
	XSetClassHint(display, window, hint);
	XFree(hint);

	XMapRaised(display, window);

	bool close;
	XEvent e;
	while (!close && entry_tick()) {
		if (!XPending(display)) {
			sleep(16);
			continue;
		}

		XNextEvent(display, &e);

		switch(e.type) {
		case ButtonPress:
			close = true;
			break;
		case ConfigureNotify:
			s_window.width = e.xconfigure.width;
			s_window.height = e.xconfigure.height;
			break;
		}
	}

	entry_shutdown();

	XUnmapWindow(display, window);
	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}

#endif
