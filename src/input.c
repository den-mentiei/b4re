#include "input.h"

#include <assert.h>
#include <string.h> // memcpy

#include "entry/entry.h"

// TODO: Introduce vec2_t ?
// TODO: @robustness Assert passed button - as it is used as an index.

static const float DRAG_THRESHOLD = 6.0f;

static struct {
	float x,      y;
	float prev_x, prev_y;
	float dx,     dy;

	float drag_start_x[ENTRY_BUTTON_COUNT];
	float drag_start_y[ENTRY_BUTTON_COUNT];
	float drag_distance_sqr[ENTRY_BUTTON_COUNT];
	float drag_dx[ENTRY_BUTTON_COUNT];
	float drag_dy[ENTRY_BUTTON_COUNT];
	bool  curr[ENTRY_BUTTON_COUNT];
	bool  prev[ENTRY_BUTTON_COUNT];
	bool  pressed[ENTRY_BUTTON_COUNT];
	bool  released[ENTRY_BUTTON_COUNT];
	bool  clicked[ENTRY_BUTTON_COUNT];
} s_ctx;

void input_update() {
	entry_mouse_position(&s_ctx.x, &s_ctx.y);
	s_ctx.dx     = s_ctx.x - s_ctx.prev_x;
	s_ctx.dy     = s_ctx.y - s_ctx.prev_y;
	s_ctx.prev_x = s_ctx.x;
	s_ctx.prev_y = s_ctx.y;

	for (size_t i = ENTRY_BUTTON_LEFT; i < ENTRY_BUTTON_COUNT; ++i) {
		s_ctx.curr[i]     = entry_mouse_pressed(i);
		s_ctx.pressed[i]  = !s_ctx.prev[i] &&  s_ctx.curr[i];
		s_ctx.released[i] =  s_ctx.prev[i] && !s_ctx.curr[i];
		s_ctx.prev[i]     = s_ctx.curr[i];

		if (s_ctx.pressed[i]) {
			s_ctx.drag_start_x[i]      = s_ctx.x;
			s_ctx.drag_start_y[i]      = s_ctx.y;
			s_ctx.drag_dx[i]           = 0.0f;
			s_ctx.drag_dy[i]           = 0.0f;
			s_ctx.drag_distance_sqr[i] = 0.0f;
		} else if (s_ctx.curr[i]) {
			const float dx             = s_ctx.x - s_ctx.drag_start_x[i];
			const float dy             = s_ctx.y - s_ctx.drag_start_y[i];
			s_ctx.drag_dx[i]           = dx;
			s_ctx.drag_dy[i]           = dy;
			s_ctx.drag_distance_sqr[i] = dx * dx + dy * dy;
		}

		if (s_ctx.released[i]) {
			s_ctx.clicked[i] = s_ctx.drag_distance_sqr[i] < DRAG_THRESHOLD * DRAG_THRESHOLD;
		} else {
			s_ctx.clicked[i] = false;
		}
	}
}

bool input_button_pressed(input_button_t b) {
	return s_ctx.pressed[b];
}

bool input_button_released(input_button_t b) {
	return s_ctx.released[b];
}

bool input_button_down(input_button_t b) {
	return s_ctx.curr[b];
}

bool input_button_clicked(input_button_t b) {
	return s_ctx.clicked[b];
}

bool input_dragging(input_button_t b) {
	if (!s_ctx.curr[b]) return false;
	return s_ctx.drag_distance_sqr[b] >= DRAG_THRESHOLD * DRAG_THRESHOLD;
}

void input_drag_delta(input_button_t b, float* x, float* y) {
	assert(x);
	assert(y);

	if (input_dragging(b)) {
		*x = s_ctx.drag_dx[b];
		*y = s_ctx.drag_dy[b];
	} else {
		*x = 0.0f;
		*y = 0.0f;
	}
}

void input_position(float* x, float* y) {
	entry_mouse_position(x, y);
}
