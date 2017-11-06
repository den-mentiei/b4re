#include "input.h"

#include <assert.h>
#include <string.h> // memcpy

#include "entry/entry.h"

static struct {
	float x, y;
	bool curr[ENTRY_BUTTON_COUNT];
	bool prev[ENTRY_BUTTON_COUNT];
} s_ctx;

void input_update() {
	memcpy(s_ctx.prev, s_ctx.curr, sizeof(s_ctx.prev));

	entry_mouse_position(&s_ctx.x, &s_ctx.y);
	for (size_t i = ENTRY_BUTTON_LEFT; i < ENTRY_BUTTON_COUNT; ++i) {
		s_ctx.curr[i] = entry_mouse_pressed(i);
	}
}

// Returns *true* if the user pressed the button during this frame, of *false* otherwise.
bool input_button_pressed(input_button_t b) {
	return s_ctx.curr[b] && !s_ctx.prev[b];
}

// Returns *true* if the user released the button during this frame, of *false* otherwise.
bool input_button_released(input_button_t b) {
	return !s_ctx.curr[b] && s_ctx.prev[b];
}

void input_position(float* x, float* y) {
	entry_mouse_position(x, y);
}
