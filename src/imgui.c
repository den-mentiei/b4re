#include "imgui.h"

#include <assert.h>

#include "input.h"
#include "render.h"

static struct {
	float x, y;
	bool  down;

	// Hot item is the one that's below the mouse cursor.
	int64_t hot;
	// Active item is the one you're currently interacting with.
	int64_t active;
} s_ctx;

static bool is_mouse_hit(float x, float y, float w, float h) {
	const float mx = s_ctx.x;
	const float my = s_ctx.y;
	return mx >= x && mx < x + w && my >= y && my < y + h;
}

void imgui_update() {
	s_ctx.hot = 0;

	if (input_button_pressed(INPUT_BUTTON_LEFT)) {
		s_ctx.down = true;
	} else if (input_button_released(INPUT_BUTTON_LEFT)) {
		s_ctx.down = false;
	}

	input_position(&s_ctx.x, &s_ctx.y);
}

void imgui_post_update() {
	if (!s_ctx.down) {
		s_ctx.active = 0;
	} else if (s_ctx.active == 0) {
		s_ctx.active = -1;
	}
}

static const color_t COLOR_NORMAL = RENDER_COLOR(255, 255, 255);
static const color_t COLOR_HOT    = RENDER_COLOR(  0, 255,   5);
static const color_t COLOR_ACTIVE = RENDER_COLOR(255,   0,   0);

bool imgui_button(int64_t id, const struct sprite_t* s, float x, float y, int w, int h) {
	assert(id > 0);

	if (is_mouse_hit(x, y, w, h)) {
		s_ctx.hot = id;
		if (s_ctx.active == 0 && s_ctx.down) {
			s_ctx.active = id;
		}
	}

	if (s_ctx.hot == id) {
		if (s_ctx.active == id) {
			// hot & active.
			render_sprite_colored(s, x, y, COLOR_ACTIVE);
		} else {
			// just hot.
			render_sprite_colored(s, x, y, COLOR_HOT);
		}
	} else {
		render_sprite_colored(s, x, y, COLOR_NORMAL);
	}

	return !s_ctx.down && s_ctx.hot == id && s_ctx.active == id;
}
