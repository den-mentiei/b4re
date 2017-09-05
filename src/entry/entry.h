#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	void* display;
	void* window;
	uint16_t width;
	uint16_t height;
} entry_window_info_t;

const entry_window_info_t* entry_get_window();

// Implemented by the user.

bool entry_init(int argc, const char* argv[]);
bool entry_tick();
void entry_shutdown();
