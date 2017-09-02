#ifdef BR_PLATFORM_LINUX

#include "entry.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <time.h> // nanosleep
#include <X11/Xlib.h>

// TODO: Check for errors.

static void sleep(uint32_t msec) {
	struct timespec req = { (time_t)msec/1000, (long)((msec % 1000) * 1000000) };
	struct timespec rem = { 0, 0 };
	nanosleep(&req, &rem);
}

int main(int argc, const char* argv[]) {
	if (!entry_init(argc, argv))
		return 1;

	XInitThreads();

	Display* display = XOpenDisplay(NULL);
	const int screen = DefaultScreen(display);

	const unsigned int w = 500;
	const unsigned int h = 300;

	Window window = XCreateSimpleWindow(
		display, DefaultRootWindow(display),
		0, 0, w, h,
		0, WhitePixel(display, screen), BlackPixel(display, screen));

	XSelectInput(display, window, ButtonPressMask);
	XStoreName(display, window, "b4re");

	XMapRaised(display, window);

	bool close_requested;
	XEvent e;
	while (!close_requested && entry_tick()) {
		if (!XPending(display)) {
			sleep(16);
			continue;
		}

		XNextEvent(display, &e);

		switch(e.type) {
		case ButtonPress:
			close_requested = true;
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
