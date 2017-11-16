#include "travel_map.h"

#include <assert.h>
#include <stdio.h> // snprintf

#include "game.h"
#include "log.h"
#include "session.h"
#include "input.h"
#include "render.h"
#include "render.inl"
#include "render_text.h"
#include "generated/assets.h"

static struct {
	float start_x;
	float start_y;
	float last_x;
	float last_y;

	float map_x;
	float map_y;
	int   tile_x;
	int   tile_y;

	bool scrolling;
} s_ctx;

typedef struct rect_t {
	float x, y;
	float w, h;
} rect_t;

static inline bool is_in_rect(float x, float y, const rect_t* rect) {
	assert(rect);

	return x >= rect->x && x < rect->x + rect->w
		&& y >= rect->y && y < rect->y + rect->h;
}

static void start_scroll() {
	assert(!s_ctx.scrolling);

	float x, y;
	input_position(&x, &y);

	s_ctx.start_x   = x;
	s_ctx.start_y   = y;
	s_ctx.last_x    = x;
	s_ctx.last_y    = y;
	s_ctx.scrolling = true;
}

static void stop_scroll() {
	assert(s_ctx.scrolling);
	s_ctx.scrolling = false;
}

static const size_t VIEW_TILES   = 8;
static const size_t PLANE_TILES  = 256;
static const float  TILE         = 64.0f;
static const float  VIEW_SIZE    = VIEW_TILES * TILE;

static void update_scroll() {
	float x, y;
	input_position(&x, &y);

	const float dx = x - s_ctx.last_x;
	const float dy = y - s_ctx.last_y;

	float new_x  = s_ctx.map_x + dx;
	float new_y  = s_ctx.map_y + dy;
	int   new_tx = s_ctx.tile_x;
	int   new_ty = s_ctx.tile_y;

	if (new_x < -TILE) {
		new_x  += TILE;
		new_tx += 1;
	} else if (new_x + VIEW_SIZE > VIEW_SIZE) {
		new_x  -= TILE;
		new_tx -= 1;
	}
	if (new_y < -TILE) {
		new_y  += TILE;
		new_ty += 1;
	} else if (new_y + VIEW_SIZE > VIEW_SIZE) {
		new_y  -= TILE;
		new_ty -= 1;
	}

	// Snapping.
	if (new_tx < 0) {
		new_x  = 0;
		new_tx = 0;
	} else if (new_tx + VIEW_TILES > PLANE_TILES) {
		new_x  = -TILE;
		new_tx = PLANE_TILES - VIEW_TILES;
	}

	if (new_ty < 0) {
		new_y  = 0;
		new_ty = 0;
	} else if (new_ty + VIEW_TILES > PLANE_TILES) {
		new_y  = -TILE;
		new_ty = PLANE_TILES - VIEW_TILES;
	}

	s_ctx.map_x  = new_x;
	s_ctx.map_y  = new_y;
	s_ctx.tile_x = new_tx;
	s_ctx.tile_y = new_ty;

	s_ctx.last_x = x;
	s_ctx.last_y = y;
}

static void render_scroll() {
	const render_text_t DEBUG_TEXT = {
		.font    = "regular",
		.size_pt = 16.0f,
		.color   = render_color(255, 255, 255),
		.align   = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE
	};

	char buf[64];

	const float ox = 32.0f;
	const float oy = 32.0f;
							  
	for (size_t i = 0; i < VIEW_TILES; ++i) {
		for (size_t j = 0; j < VIEW_TILES; ++j) {
			const size_t tx = s_ctx.tile_x + i;
			const size_t ty = s_ctx.tile_y + j;
			const float  x  = ox + s_ctx.map_x + TILE * i;
			const float  y  = oy + s_ctx.map_y + TILE * j;

			snprintf(buf, 64, "%zu,%zu", tx, ty);
			render_sprite(assets_sprites()->common.sign_green_dark, x, y);
			render_text(buf, x + TILE * 0.5f, y + TILE * 0.5f, &DEBUG_TEXT);
		}
	}

	const float gx = ox + s_ctx.map_x;
	const float gy = oy + s_ctx.map_y;
	render_sprite(assets_sprites()->travel_map.mapnet, gx,          gy);
	render_sprite(assets_sprites()->travel_map.mapnet, gx,          gy + 256.0f);
	render_sprite(assets_sprites()->travel_map.mapnet, gx + 256.0f, gy);
	render_sprite(assets_sprites()->travel_map.mapnet, gx + 256.0f, gy + 256.0f);
}

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {
	if (!s_ctx.scrolling && input_button_pressed(INPUT_BUTTON_LEFT)) {
		start_scroll();
		return;
	}

	if (s_ctx.scrolling) {
		if (input_button_released(INPUT_BUTTON_LEFT)) {
			stop_scroll();
		} else {
			update_scroll();
		}
		return;
	}

	if (input_button_released(INPUT_BUTTON_LEFT)) {
		if (session_current()) {
			session_end();
		}
	}

	if (!session_current()) {
		game_state_switch(GAME_STATE_LOGIN);
	}
}

typedef struct {
	const struct sprite_t* icon; 
	const struct sprite_t* body;
	const struct sprite_t* edge;

	const struct sprite_t* time;

	const struct sprite_t* segment_empty;
	const struct sprite_t* segment_full;

	const struct sprite_t* booster_half;
	const struct sprite_t* booster_full;

	const struct sprite_t* regen[10];

	uint32_t color;
	uint32_t emphasis_color;

	render_text_t text_params;
} indicator_rendering_t;

static void resource_indicator_render(const resource_t* res, const indicator_rendering_t* sprites, float x, float y) {
	assert(res);
	assert(sprites);

	const size_t NUM_SEGMENTS = 12;

	render_sprite(sprites->regen[0], x, y);

	y += 32.0f;
	render_sprite(sprites->time, x, y);

	char buf[8];
	snprintf(buf, 8, "%u", res->regen_rate);
	render_text(buf, x + 16.0f, y + 16.0f, &sprites->text_params);

	y += 32.0f;
	for (size_t i = 0; i < NUM_SEGMENTS; ++i) {
		render_sprite(sprites->segment_full, x, y);
		y += 32.0f;
	}
}

static void render_value_max(uint8_t value, uint8_t max, float x, float y, const indicator_rendering_t* params) {
	assert(params);

	char buf[8] = {0};

	render_text_t text_params = params->text_params;
	float w;
	text_params.bounds_w = &w;
	text_params.align    = RENDER_TEXT_ALIGN_LEFT | RENDER_TEXT_ALIGN_MIDDLE;
	
	int significant = snprintf(buf, 8, "%u", value);
	int zeros       = 3 - significant;
	if (zeros > 0) {
		snprintf(buf, 8, "%0*d", zeros, 0);
		text_params.color = params->color;
		render_text(buf, x, y, &text_params);
		x += w;
	}

	snprintf(buf, 8, "%d", value);
	text_params.color = params->emphasis_color;
	render_text(buf, x, y, &text_params);
	x += w;

	text_params.color = params->color;
	render_text("/", x, y, &text_params);
	x += w;

	snprintf(buf, 8, "%03d", max);
	text_params.color = params->color;
	render_text(buf, x, y, &text_params);
}

static void resource_indicator_value_render(const resource_t* res, const indicator_rendering_t* params, float x, float y, bool reversed) {
	assert(res);
	assert(params);

	const size_t NUM_BODY_SEGMENTS = 3;
	const float dx = reversed ? -32.0f : 32.0f;

	const float tx = x + (reversed ? (dx * NUM_BODY_SEGMENTS) : 32.0f + 8.0f);
	const float ty = y + 16.0f;

	render_sprite(params->icon, x, y);
	x += dx;

	for (size_t i = 0; i < NUM_BODY_SEGMENTS; ++i) {
		render_sprite(params->body, x, y);
		x += dx;
	}
	
	render_sprite(params->edge, x, y);

	render_value_max(res->value, res->max, tx, ty, params);
}

static void coordinates_render(float x, float y, uint8_t tiles_x, uint8_t tiles_y) {
	render_sprite(assets_sprites()->travel_map.greek_letter_black_beta, x,         y);
	render_sprite(assets_sprites()->travel_map.dice_5p_brown,           x + 32.0f, y);

	char buf[8] = {0};

	float w;
	render_text_t text_params = {
		.font       = "regular",
		.size_pt    = 30.0f,
		.bounds_w   = &w,
		.align      = RENDER_TEXT_ALIGN_LEFT | RENDER_TEXT_ALIGN_MIDDLE,
	};
	
	x += 32.0f * 2 + 8.0f;
	y += 16.0f;

	const char* we = tiles_x <= 127 ? "W" : "E";
	const char* ns = tiles_y <= 127 ? "N" : "S";
	const int   dx = tiles_x <= 127 ? 128 - tiles_x : tiles_x - 127;
	const int   dy = tiles_y <= 127 ? 128 - tiles_y : tiles_y - 127;

	text_params.color = render_color(255, 255, 255);
	render_text(ns, x, y, &text_params);
	x += w;

	int significant = snprintf(buf, 8, "%u", dx);
	int zeros       = 3 - significant;
	if (zeros > 0) {
		snprintf(buf, 8, "%0*d", zeros, 0);
		text_params.color = render_color(48, 48, 54);
		render_text(buf, x, y, &text_params);
		x += w;
	}

	snprintf(buf, 8, "%d", dx);
	text_params.color = render_color(143, 54, 33);
	render_text(buf, x, y, &text_params);
	x += w;

	x += 8.0f;

	text_params.color = render_color(255, 255, 255);
	render_text(we, x, y, &text_params);
	x += w;

	significant = snprintf(buf, 8, "%u", dy);
	zeros       = 3 - significant;
	if (zeros > 0) {
		snprintf(buf, 8, "%0*d", zeros, 0);
		text_params.color = render_color(48, 48, 54);
		render_text(buf, x, y, &text_params);
		x += w;
	}

	snprintf(buf, 8, "%d", dy);
	text_params.color = render_color(143, 54, 33);
	render_text(buf, x, y, &text_params);
	x += w;
}

static void resources_render() {
	const indicator_rendering_t mind_rendering = {
		.color          = render_color(72, 107, 128),
		.emphasis_color = render_color(86, 199, 250),

		.text_params = {
			.font       = "regular",
			.size_pt    = 26.0f,
			.spacing_pt = 1.0f,
			.color      = render_color(0, 0, 0),
			.align      = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE,
		},

		.icon = assets_sprites()->travel_map.indicator_mind_icon,
		.body = assets_sprites()->travel_map.indicator_mind_body,
		.edge = assets_sprites()->travel_map.indicator_mind_edge,

		.time = assets_sprites()->travel_map.indicator_mind_regenerate_time,

		.segment_empty = assets_sprites()->travel_map.indicator_mind_regenerate_empty,
		.segment_full  = assets_sprites()->travel_map.indicator_mind_regenerate_full,

		.booster_half = assets_sprites()->travel_map.indicator_mind_buster_full,
		.booster_full = assets_sprites()->travel_map.indicator_mind_buster_half,

		.regen = {
			assets_sprites()->travel_map.indicator_mind_regenerate_0s,
			assets_sprites()->travel_map.indicator_mind_regenerate_1s,
			assets_sprites()->travel_map.indicator_mind_regenerate_2s,
			assets_sprites()->travel_map.indicator_mind_regenerate_3s,
			assets_sprites()->travel_map.indicator_mind_regenerate_4s,
			assets_sprites()->travel_map.indicator_mind_regenerate_5s,
			assets_sprites()->travel_map.indicator_mind_regenerate_6s,
			assets_sprites()->travel_map.indicator_mind_regenerate_7s,
			assets_sprites()->travel_map.indicator_mind_regenerate_8s,
			assets_sprites()->travel_map.indicator_mind_regenerate_9s,
		}
	};
	const indicator_rendering_t matter_rendering = {
		.color          = render_color(117, 47, 44),
		.emphasis_color = render_color(236, 74, 39),

		.text_params = {
			.font    = "regular",
			.size_pt = 26.0f,
			.color   = render_color(0, 0, 0),
			.align   = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE,
		},

		.icon = assets_sprites()->travel_map.indicator_matter_icon,
		.body = assets_sprites()->travel_map.indicator_matter_body,
		.edge = assets_sprites()->travel_map.indicator_matter_edge,

		.time = assets_sprites()->travel_map.indicator_matter_regenerate_time,

		.segment_empty = assets_sprites()->travel_map.indicator_matter_regenerate_empty,
		.segment_full  = assets_sprites()->travel_map.indicator_matter_regenerate_full,

		.booster_half = assets_sprites()->travel_map.indicator_matter_buster_full,
		.booster_full = assets_sprites()->travel_map.indicator_matter_buster_half,

		.regen = {
			assets_sprites()->travel_map.indicator_matter_regenerate_0s,
			assets_sprites()->travel_map.indicator_matter_regenerate_1s,
			assets_sprites()->travel_map.indicator_matter_regenerate_2s,
			assets_sprites()->travel_map.indicator_matter_regenerate_3s,
			assets_sprites()->travel_map.indicator_matter_regenerate_4s,
			assets_sprites()->travel_map.indicator_matter_regenerate_5s,
			assets_sprites()->travel_map.indicator_matter_regenerate_6s,
			assets_sprites()->travel_map.indicator_matter_regenerate_7s,
			assets_sprites()->travel_map.indicator_matter_regenerate_8s,
			assets_sprites()->travel_map.indicator_matter_regenerate_9s,
		}
	};

	// Resources
	const resource_t* mind   = &session_current()->player.mind;
	const resource_t* matter = &session_current()->player.matter;
	resource_indicator_render(mind,     &mind_rendering,           0.0f, 32.0f);
	resource_indicator_render(matter, &matter_rendering, 512.0f - 32.0f, 32.0f);

	resource_indicator_value_render(mind,     &mind_rendering,           0.0f, 512.0f - 32.0f, false);
	resource_indicator_value_render(matter, &matter_rendering, 512.0f - 32.0f, 512.0f - 32.0f, true);
}

static void render_movement() {
	const uint8_t x = 3, y = 3;
	/* render_sprite(assets_sprites()->avatars.avatar_man2, 32.0f + 64.0f * x, 32.0f + 64.0f * y); */
}

void states_travel_map_render(uint16_t width, uint16_t height, float dt) {
	if (!session_current()) return;

	// TODO: Render map view.
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);
	render_movement();

	render_scroll();

	// Map chrome.
	render_sprite(assets_sprites()->travel_map.black_map_frame,    0.0f,  0.0f);
	render_sprite(assets_sprites()->travel_map.atlas_frame,       32.0f, 32.0f);
	render_sprite(assets_sprites()->common.header,                32.0f,  0.0f);
	static const render_text_t title_params = {
		.font    = "regular",
		.size_pt = 26.0f,
		.align   = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE,
		.shadow  = true
	};
	render_text("Hello, sailor!", 512.0f * 0.5f, 16.0f, &title_params);

	// Planets influence
	render_sprite(assets_sprites()->travel_map.greek_letter_black_alpha, 0.0f,           0.0f);
	render_sprite(assets_sprites()->travel_map.greek_letter_black_omega, 512.0f - 32.0f, 0.0f);

	// TODO: Should be shown only if player is not visible in the current view.
	/* render_sprite(assets_sprites()->travel_map.button_compass_n, 512.0f * 0.5f - 32.0f, 512.0f - 64.0f - 32.0f); */
	resources_render();
	coordinates_render(5 * 32.0f, 512.0f - 32.0f, s_ctx.tile_x, s_ctx.tile_y);
}
