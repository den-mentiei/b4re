#ifdef BR_PLATFORM_MACOS

#include "entry.h"

#include "log.h"

static entry_window_info_t s_window;

const entry_window_info_t* entry_get_window() {
	return &s_window;
}

int main(int argc, const char* argv[]) {
	const uint16_t w = 600;
	const uint16_t h = 400;

	// s_window.display = display;
	// s_window.window = (void*)(uintptr_t)window;
	s_window.width = w;
	s_window.height = h;

	if (!entry_init(argc, argv))
		// TODO: cleanup.
		return 1;

	entry_tick(1000/60.0f);
	
	entry_shutdown();

	return 0;
}

#endif
