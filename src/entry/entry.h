#pragma once

#include <stdint.h>
#include <stdbool.h>

#define ENTRY_WINDOW_WIDTH  1024.0f
#define ENTRY_WINDOW_HEIGHT 512.0f

typedef struct {
	void* display;
	void* window;
	uint16_t width;
	uint16_t height;
} entry_window_info_t;

const entry_window_info_t* entry_get_window();

// Implemented by the user.

bool entry_init(int32_t argc, const char* argv[]);
bool entry_tick(float dt);
void entry_shutdown();
