#include "travel_map.h"

#include <assert.h>
#include <stdio.h>  // snprintf
#include <stdlib.h> // abs

#include "game.h"
#include "world.h"
#include "log.h"
#include "session.h"
#include "imgui.h"
#include "input.h"
#include "render.h"
#include "render.inl"
#include "render_text.h"
#include "generated/assets.h"

static const size_t VIEW_TILES       = 7;
static const size_t VIEW_TILES_PAD   = VIEW_TILES + 1;
static const size_t OFFSET_TO_CENTER = VIEW_TILES / 2;
static const size_t PLANE_TILES      = 256;
static const float  TILE             = 64.0f;
static const float  VIEW_SIZE        = VIEW_TILES * TILE;
static const float  VIEW_OFFSET      = 0.5f * TILE;
static const float  SCREEN_SIZE      = 8    * TILE;

#define MAX_PATH_LENGTH 100

static struct {
	float   map_x;
	float   map_y;
	int32_t tile_x;
	int32_t tile_y;

	size_t num_steps;
	struct {
		int32_t tx;
		int32_t ty;
		uint8_t price;
	} steps[MAX_PATH_LENGTH];

	int32_t selector_x;
	int32_t selector_y;
	bool    has_selector;

	bool is_drawing;
	bool is_scrolling;
} s_ctx;

// HELPERS
// =======

static bool is_in_rect(float x, float y, float w, float h, float px, float py) {
	return px >= x && px < x + w &&
	       py >= y && py < y + h;
}

static bool is_on_plane(int32_t tx, int32_t ty) {
	return tx >= 0 && tx < WORLD_PLANE_SIZE &&
	       ty >= 0 && ty < WORLD_PLANE_SIZE;
}

static bool are_neighbours(int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1) {
	int32_t dx = abs(tx0 - tx1);
	int32_t dy = abs(ty0 - ty1);
	return is_on_plane(tx0, ty0) && is_on_plane(tx1, ty1) && ((dx + dy) == 1);
}

static bool are_diagonal_only_neighbours(int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1) {
	int32_t dx = abs(tx0 - tx1);
	int32_t dy = abs(ty0 - ty1);
	return is_on_plane(tx0, ty0) && is_on_plane(tx1, ty1) && dx == 1 && dy == 1;
}

static bool is_tile_dragged(int32_t tx, int32_t ty) {
	if (input_dragging(INPUT_BUTTON_LEFT)) {
		const float ox = VIEW_OFFSET + s_ctx.map_x;
		const float oy = VIEW_OFFSET + s_ctx.map_y;
		const float x  = ox + TILE * (tx - s_ctx.tile_x);
		const float y  = oy + TILE * (ty - s_ctx.tile_y);

		float ix, iy;
		input_position(&ix, &iy);

		if (is_in_rect(x, y, TILE, TILE, ix, iy)) {
			return true;
		}
	}
	return false;
}

// PATH MANAGEMENT
// ===============

static bool path_contains(int32_t tx, int32_t ty) {
	for (size_t i = 0; i < s_ctx.num_steps; ++i) {
		if (s_ctx.steps[i].tx == tx && s_ctx.steps[i].ty == ty) return true;
	}
	return false;
}

static void path_reset() {
	s_ctx.num_steps = 0;
}

static bool path_reset_to(int32_t tx, int32_t ty) {
	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;
	if (tx == px && ty == py) {
		path_reset();
		return true;
	}

	size_t i = 0;
	while (i < s_ctx.num_steps) {
		if (s_ctx.steps[i].tx == tx && s_ctx.steps[i].ty == ty) {
			s_ctx.num_steps = i + 1;
			return true;
		}
		++i;
	}

	return false;
}

static void path_step(int32_t tx, int32_t ty) {
	if (!path_reset_to(tx, ty)) {
		// TODO: Calculate price, validate it, etc.
		s_ctx.steps[s_ctx.num_steps].tx    = tx;
		s_ctx.steps[s_ctx.num_steps].ty    = ty;
		s_ctx.steps[s_ctx.num_steps].price = 1;
		++s_ctx.num_steps;
	}
}

static void path_step_diagonally(int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1) {
	assert(are_diagonal_only_neighbours(tx0, ty0, tx1, ty1));

	const int32_t dx = tx1 - tx0;
	// const int32_t dy = ty1 - ty0;

	// Step 1
	const int32_t through_tx = tx1 - dx;
	const int32_t through_ty = ty1;

	path_step(through_tx, through_ty);
	path_step(tx1,        ty1);

	// Step 2
	/* const int32_t through_tx = tx1; */
	/* const int32_t through_ty = ty1 - dy; */
	// TODO:
}

static void path_input(int32_t tx, int32_t ty) {
	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;
	
	const int32_t sx = s_ctx.num_steps > 0 ? s_ctx.steps[s_ctx.num_steps - 1].tx : px;
	const int32_t sy = s_ctx.num_steps > 0 ? s_ctx.steps[s_ctx.num_steps - 1].ty : py;

	if (are_diagonal_only_neighbours(sx, sy, tx, ty)) {
		path_step_diagonally(sx, sy, tx, ty);
	} else if (are_neighbours(sx, sy, tx, ty)) {
		path_step(tx, ty);
	} else {
		path_reset_to(tx, ty);
	}
}

static bool path_can_start_from(int32_t tx, int32_t ty) {
	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;
	if (s_ctx.selector_x == px && s_ctx.selector_y == py) {
		if (tx == px && ty == py) return true;
	}

	return path_contains(tx, ty);
}

// SCROLL MANAGEMENT
// =================

static void scroll_update() {
	float x, y;
	input_position(&x, &y);

	float dx, dy;
	input_position_delta(&dx, &dy);

	float   new_x  = s_ctx.map_x + dx;
	float   new_y  = s_ctx.map_y + dy;
	int32_t new_tx = s_ctx.tile_x;
	int32_t new_ty = s_ctx.tile_y;

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
	} else if (new_tx + VIEW_TILES_PAD > PLANE_TILES) {
		new_x  = -TILE;
		new_tx = PLANE_TILES - VIEW_TILES_PAD;
	}
	if (new_ty < 0) {
		new_y  = 0;
		new_ty = 0;
	} else if (new_ty + VIEW_TILES_PAD > PLANE_TILES) {
		new_y  = -TILE;
		new_ty = PLANE_TILES - VIEW_TILES_PAD;
	}

	s_ctx.map_x  = new_x;
	s_ctx.map_y  = new_y;
	s_ctx.tile_x = new_tx;
	s_ctx.tile_y = new_ty;
}

// MAP VIEW
// ========

static bool map_view_pick_tile(float x, float y, int32_t* tx, int32_t* ty) {
	assert(tx);
	assert(ty);

	if (!is_in_rect(VIEW_OFFSET, VIEW_OFFSET, VIEW_SIZE, VIEW_SIZE, x, y)) {
		return false;
	}

	const float ox = VIEW_OFFSET + s_ctx.map_x;
	const float oy = VIEW_OFFSET + s_ctx.map_y;

	for (size_t i = 0; i < VIEW_TILES_PAD; ++i) {
		for (size_t j = 0; j < VIEW_TILES_PAD; ++j) {
			const float px = ox + TILE * i;
			const float py = oy + TILE * j;

			if (is_in_rect(px, py, TILE, TILE, x, y)) {
				*tx = s_ctx.tile_x + i;
				*ty = s_ctx.tile_y + j;
				return true;
			}
		}
	}

	return false;
}

static void map_view_update() {
	float x, y;
	input_position(&x, &y);

	int32_t tx, ty;
	if (map_view_pick_tile(x, y, &tx, &ty)) {
		if (input_button_clicked(INPUT_BUTTON_LEFT)) {
			if (path_contains(tx, ty)) {
				// TODO: Split & walk.
				log_info("[travel map] Walking to %d, %d", tx, ty);
			} else if (session_current()->world.locations[tx][ty].is_hidden) {
				session_reveal(tx, ty);
			} else {
				s_ctx.selector_x = tx;
				s_ctx.selector_y = ty;
			}
		}

		if (!s_ctx.is_drawing && input_dragging(INPUT_BUTTON_LEFT)) {
			s_ctx.is_drawing = path_can_start_from(tx, ty);
		}
		if (s_ctx.is_drawing) {
			s_ctx.is_drawing = input_dragging(INPUT_BUTTON_LEFT);
			path_input(tx, ty);
		} else {
			s_ctx.is_scrolling = input_dragging(INPUT_BUTTON_LEFT);
			if (s_ctx.is_scrolling) {
				scroll_update();
			}
		}
	}
}

static const struct sprite_t* lookup_terrain_sprite(uint8_t t) {
#define SPRITE(s) assets_sprites()->travel_map.atlas_tiled_##s
	const struct { world_terrain_t t; const struct sprite_t* s; } TERRAIN_SPRITES[] = {
		{ TERRAIN_DEFAULT,      SPRITE(core)         },
		{ TERRAIN_ROCK_WATER,   SPRITE(water)        },
		{ TERRAIN_ROCK_SOLID,   SPRITE(rock_solid)   },
		{ TERRAIN_ROCK,         SPRITE(rock)         },
		{ TERRAIN_ROCK_SAND,    SPRITE(rock_sand)    },
		{ TERRAIN_WILD,         SPRITE(wild)         },
		{ TERRAIN_GRASS,        SPRITE(grass)        },
		{ TERRAIN_EARTH,        SPRITE(earth)        },
		{ TERRAIN_CLAY,         SPRITE(clay)         },
		{ TERRAIN_SAND,         SPRITE(sand)         },
		{ TERRAIN_WATER,        SPRITE(water)        },
		{ TERRAIN_WATER_BOTTOM, SPRITE(water_bottom) },
		{ TERRAIN_WATER_DEEP,   SPRITE(water_deep)   },
	};
#undef SPRITE
	const size_t N = sizeof(TERRAIN_SPRITES) / sizeof(TERRAIN_SPRITES[0]);
	for (size_t i = 0 ; i < N; ++i) {
		if (TERRAIN_SPRITES[i].t == t) return TERRAIN_SPRITES[i].s;
	}
	return TERRAIN_SPRITES[0].s;
}

static void render_incognitta_shade(size_t tx, size_t ty, float x, float y) {
#define LOOKUP(dx, dy) session_current()->world.locations[tx + (dx)][ty + (dy)].is_hidden
	if (tx == 0 || LOOKUP(-1, 0)) {
		render_sprite(assets_sprites()->travel_map.shade_incognitta_left, x, y);
	}
	if (tx == WORLD_PLANE_SIZE - 1 || LOOKUP(1, 0)) {
		render_sprite(assets_sprites()->travel_map.shade_incognitta_right, x, y);
	}
	if (ty == 0 || LOOKUP(0, -1)) {
		render_sprite(assets_sprites()->travel_map.shade_incognitta_top, x, y);
	}
	if (ty == WORLD_PLANE_SIZE - 1 || LOOKUP(0, 1)) {
		render_sprite(assets_sprites()->travel_map.shade_incognitta_bottom, x, y);
	}
#undef LOOKUP
}

static void map_view_render() {
	const render_text_t DEBUG_TEXT = {
		.font    = "regular",
		.size_pt = 16.0f,
		.color   = render_color(255, 255, 255),
		.align   = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE
	};

	const float ox = VIEW_OFFSET + s_ctx.map_x;
	const float oy = VIEW_OFFSET + s_ctx.map_y;

	for (size_t i = 0; i < VIEW_TILES_PAD; ++i) {
		for (size_t j = 0; j < VIEW_TILES_PAD; ++j) {
			const size_t tx = s_ctx.tile_x + i;
			const size_t ty = s_ctx.tile_y + j;
			const float  x  = ox + TILE * i;
			const float  y  = oy + TILE * j;

			const world_location_t* loc = &session_current()->world.locations[tx][ty];
			const render_tile_t test_tile = {
				.tile_w = TILE,
				.tile_h = TILE,
				.tile_x = tx % 7,
				.tile_y = ty % 7,
			};
			if (!loc->has_data || loc->is_hidden) {
				render_tile(assets_sprites()->travel_map.atlas_tiled_warfog, x, y, &test_tile);
			} else {
				render_tile(lookup_terrain_sprite(loc->terrain), x, y, &test_tile);
				render_incognitta_shade(tx, ty, x, y);
			}

			char buf[64];
			snprintf(buf, 64, "%zu,%zu", tx, ty);
			render_text(buf, x + TILE * 0.5f, y + TILE * 0.5f, &DEBUG_TEXT);
		}
	}
}

static void reveal_render() {
	const struct { int8_t dx; int8_t dy; } LOOKUP[] = {
		{ -1,  0 },
		{  1,  0 },
		{  0, -1 },
		{  0,  1 },
	};
	const size_t N = sizeof(LOOKUP) / sizeof(LOOKUP[0]);

	const float   ox = VIEW_OFFSET + s_ctx.map_x;
	const float   oy = VIEW_OFFSET + s_ctx.map_y;
	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;

	for (size_t i = 0; i < N; ++i) {
		const int64_t nx = px + LOOKUP[i].dx;
		const int64_t ny = py + LOOKUP[i].dy;
		const float   x  = ox + TILE * (nx - s_ctx.tile_x) + TILE * 0.25f;
		const float   y  = oy + TILE * (ny - s_ctx.tile_y) + TILE * 0.25f;
		if (nx >= 0 && nx < WORLD_PLANE_SIZE && ny >= 0 && ny < WORLD_PLANE_SIZE &&
			session_current()->world.locations[nx][ny].is_hidden)
		{
			render_sprite(assets_sprites()->travel_map.eye_mind, x, y);
		}
	}
}

static void center_on_player() {
	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;

	s_ctx.map_x       = 0.0f;
	s_ctx.map_y       = 0.0f;
	s_ctx.tile_x      = px > OFFSET_TO_CENTER ? px - OFFSET_TO_CENTER : 0;
	s_ctx.tile_y      = py > OFFSET_TO_CENTER ? py - OFFSET_TO_CENTER : 0;
	s_ctx.selector_x  = px;
	s_ctx.selector_y  = py;
}

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {
	if (!session_current()) game_state_switch(GAME_STATE_LOGIN);
	
	if (!s_ctx.has_selector) {
		center_on_player();
		s_ctx.has_selector = true;
	}

	map_view_update();
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

	uint32_t zeros_color;
	uint32_t value_color;
	uint32_t max_color;

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

static float render_value_with_zeros(uint8_t value, float x, float y, color_t zeros, color_t emphasis, const render_text_t* text_params) {
	assert(text_params);

	char buf[8] = {0};

	float w;
	render_text_t params = *text_params;
	params.bounds_w = &w;

	int8_t significant = snprintf(buf, 8, "%u", value);
	int8_t num_zeros   = 3 - significant;
	if (num_zeros > 0) {
		snprintf(buf, 8, "%0*d", num_zeros, 0);
		params.color = zeros;
		render_text(buf, x, y, &params);
		x += w;
	}
	snprintf(buf, 8, "%d", value);
	params.color = emphasis;
	render_text(buf, x, y, &params);
	x += w;

	return x;
}

static void render_value_max(uint8_t value, uint8_t max, float x, float y, const indicator_rendering_t* params) {
	assert(params);

	render_text_t text_params = params->text_params;
	float w;
	text_params.bounds_w = &w;
	text_params.align    = RENDER_TEXT_ALIGN_LEFT | RENDER_TEXT_ALIGN_MIDDLE;
	
	x = render_value_with_zeros(value, x, y, params->zeros_color, params->value_color, &text_params);

	text_params.color = params->max_color;
	render_text("/", x, y, &text_params);
	x += w;

	render_value_with_zeros(max, x, y, params->zeros_color, params->max_color, &text_params);
}

static void resource_indicator_value_render(const resource_t* res, const indicator_rendering_t* params, float x, float y, bool reversed) {
	assert(res);
	assert(params);

	const size_t NUM_BODY_SEGMENTS = 3;
	const float dx = reversed ? -32.0f : 32.0f;

	const float tx = x + (reversed ? (dx * NUM_BODY_SEGMENTS) : 32.0f + 8.0f);
	const float ty = y + 16.0f - 2.0f;

	render_sprite(params->icon, x, y);
	x += dx;

	for (size_t i = 0; i < NUM_BODY_SEGMENTS; ++i) {
		render_sprite(params->body, x, y);
		x += dx;
	}
	
	render_sprite(params->edge, x, y);

	render_value_max(res->value, res->max, tx, ty, params);
}

static void coordinates_render(float x, float y, int8_t tiles_x, int8_t tiles_y) {
	render_sprite(assets_sprites()->travel_map.greek_letter_black_beta, x,         y);
	render_sprite(assets_sprites()->travel_map.dice_5p_brown,           x + 32.0f, y);

	float w;
	render_text_t text_params = {
		.font       = "regular",
		.size_pt    = 30.0f,
		.bounds_w   = &w,
		.align      = RENDER_TEXT_ALIGN_LEFT | RENDER_TEXT_ALIGN_MIDDLE,
	};
	const color_t zeros_color    = render_color( 48, 48, 54);
	const color_t emphasis_color = render_color(143, 54, 33); 
	
	x += 32.0f * 2 + 8.0f;
	y += 16.0f;

	const char*   we = tiles_x <= 127 ? "W" : "E";
	const char*   ns = tiles_y <= 127 ? "N" : "S";
	const int32_t dx = tiles_x <= 127 ? 128 - tiles_x : tiles_x - 127;
	const int32_t dy = tiles_y <= 127 ? 128 - tiles_y : tiles_y - 127;

	text_params.color = render_color(255, 255, 255);
	render_text(ns, x, y, &text_params);
	x += w;

	x = render_value_with_zeros(dx, x, y, zeros_color, emphasis_color, &text_params);

	x += 8.0f;

	text_params.color = render_color(255, 255, 255);
	render_text(we, x, y, &text_params);
	x += w;

	x = render_value_with_zeros(dy, x, y, zeros_color, emphasis_color, &text_params);
}

static void resources_render() {
	const indicator_rendering_t mind_rendering = {
		.zeros_color = render_color(0x0D, 0x27, 0x34),
		.value_color = render_color(0x00, 0xC8, 0xFF),
		.max_color   = render_color(0x3F, 0x6C, 0x83),

		.text_params = {
			.font       = "regular",
			.size_pt    = 28.0f,
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
		.zeros_color = render_color(0x2D, 0x07, 0x07),
		.value_color = render_color(0xFF, 0x37, 0x00),
		.max_color   = render_color(0x7E, 0x29, 0x29),

		.text_params = {
			.font    = "regular",
			.size_pt = 28.0f,
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

static struct { uint8_t t; uint8_t c; } TERRAIN_CLASS[] = {
	{ TERRAIN_DEFAULT,      TERRAIN_CLASS_DEFAULT },
	{ TERRAIN_ROCK_WATER,   TERRAIN_CLASS_ROCK    },
	{ TERRAIN_ROCK_SOLID,   TERRAIN_CLASS_ROCK    },
	{ TERRAIN_ROCK,         TERRAIN_CLASS_ROCK    },
	{ TERRAIN_ROCK_SAND,    TERRAIN_CLASS_ROCK    },
	{ TERRAIN_WILD,         TERRAIN_CLASS_WILD    },
	{ TERRAIN_GRASS,        TERRAIN_CLASS_GRASS   },
	{ TERRAIN_EARTH,        TERRAIN_CLASS_EARTH   },
	{ TERRAIN_CLAY,         TERRAIN_CLASS_CLAY    },
	{ TERRAIN_SAND,         TERRAIN_CLASS_SAND    },
	{ TERRAIN_WATER,        TERRAIN_CLASS_WATER   },
	{ TERRAIN_WATER_BOTTOM, TERRAIN_CLASS_WATER   },
	{ TERRAIN_WATER_DEEP,   TERRAIN_CLASS_WATER   },
};

static uint8_t get_terrain_class(int32_t tx, int32_t ty) {
	const size_t  N = sizeof(TERRAIN_CLASS) / sizeof(TERRAIN_CLASS[0]);
	const uint8_t t = session_current()->world.locations[tx][ty].terrain;
	for (size_t i = 0; i < N; ++i) {
		if (TERRAIN_CLASS[i].t == t) return TERRAIN_CLASS[i].c;
	}
	return TERRAIN_CLASS_DEFAULT;
}

static size_t get_arrow_index(int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1) {
	assert(tx0 != tx1 || ty0 != ty1);

	// n s w e
	// 0 1 2 3

	if (ty1 < ty0) return 0;
	if (ty1 > ty0) return 1;
	if (tx1 < tx0) return 2;
	if (tx1 > tx0) return 3;

	assert(false);
	return 0;
}

typedef struct {
	const struct sprite_t* s;
	float dx;
	float dy;
} arrow_t;
static void get_arrow(int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1, arrow_t* out) {
#define SPRITE(s)     assets_sprites()->travel_map.s
#define DIR_SPRITE(s) SPRITE(direction_##s)
#define DIRECTIONS(s) { DIR_SPRITE(north_##s), DIR_SPRITE(south_##s), DIR_SPRITE(west_##s), DIR_SPRITE(east_##s) }
	const struct {
		uint8_t c;
		// n s w e
		const struct sprite_t* arrows[4];
	} LOOKUP[] = {
		{ TERRAIN_CLASS_DEFAULT, DIRECTIONS(rock)  },
		{ TERRAIN_CLASS_ROCK,    DIRECTIONS(rock)  },
		{ TERRAIN_CLASS_WILD,    DIRECTIONS(wild)  },
		{ TERRAIN_CLASS_GRASS,   DIRECTIONS(grass) },
		{ TERRAIN_CLASS_EARTH,   DIRECTIONS(earth) },
		{ TERRAIN_CLASS_CLAY,    DIRECTIONS(clay)  },
		{ TERRAIN_CLASS_SAND,    DIRECTIONS(sand)  },
		{ TERRAIN_CLASS_WATER,   DIRECTIONS(water) },
	};
	const size_t N = sizeof(LOOKUP) / sizeof(LOOKUP[0]);
#undef SPRITE
#undef DIR_SPRITE
#undef DIRECTIONS

	// n s w e
	const struct { float dx; float dy; } ARROW_OFFSETS[] = {
		{  0.0f,        -TILE * 0.5f },
		{  0.0f,         TILE * 0.5f },
		{ -TILE * 0.5f,  0.0f },
		{  TILE * 0.5f,  0.0f },
	};

	assert(out);

	const uint8_t c = get_terrain_class(tx1, ty1);
	const size_t  a = get_arrow_index(tx0, ty0, tx1, ty1);

	out->dx = ARROW_OFFSETS[a].dx;
	out->dy = ARROW_OFFSETS[a].dy;
	
	for (size_t i = 0; i < N; ++i) {
		if (LOOKUP[i].c == c) {
			out->s = LOOKUP[i].arrows[a];
			return;
		}
	}
	out->s = LOOKUP[0].arrows[a];
}

static const struct sprite_t* get_path_point(int32_t tx, int32_t ty) {
#define POINT(s) assets_sprites()->travel_map.point_##s
	const struct {
		uint8_t c;
		const struct sprite_t* p;
	} LOOKUP[] = {
		{ TERRAIN_CLASS_DEFAULT, POINT(rock)  },
		{ TERRAIN_CLASS_ROCK,    POINT(rock)  },
		{ TERRAIN_CLASS_WILD,    POINT(wild)  },
		{ TERRAIN_CLASS_GRASS,   POINT(grass) },
		{ TERRAIN_CLASS_EARTH,   POINT(earth) },
		{ TERRAIN_CLASS_CLAY,    POINT(clay)  },
		{ TERRAIN_CLASS_SAND,    POINT(sand)  },
		{ TERRAIN_CLASS_WATER,   POINT(water) },
	};
#undef POINT
	const size_t  N = sizeof(LOOKUP) / sizeof(LOOKUP[0]);
	const uint8_t c = get_terrain_class(tx, ty);
	for (size_t i = 0; i < N; ++i) {
		if (LOOKUP[i].c == c) return LOOKUP[i].p;
	}
	return LOOKUP[0].p;
}

static void path_render_arrow(int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1) {
	const float ox = VIEW_OFFSET + s_ctx.map_x;
	const float oy = VIEW_OFFSET + s_ctx.map_y;
	const float x  = ox + TILE * (tx0 - s_ctx.tile_x);
	const float y  = oy + TILE * (ty0 - s_ctx.tile_y);

	arrow_t arrow;
	get_arrow(tx0, ty0, tx1, ty1, &arrow);
	render_sprite(arrow.s, x + arrow.dx, y + arrow.dy);
}

static void path_render() {
	if (s_ctx.num_steps == 0) return;

	const render_text_t TEXT = {
		.font    = "regular",
		.size_pt = 24.0f,
		.color   = render_color(255, 255, 255),
		.align   = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE
	};

	const float ox = VIEW_OFFSET + s_ctx.map_x;
	const float oy = VIEW_OFFSET + s_ctx.map_y;

	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;
	path_render_arrow(px, py, s_ctx.steps[0].tx, s_ctx.steps[0].ty);

	for (size_t i = 0; i < s_ctx.num_steps; ++i) {
		const int32_t tx0 = s_ctx.steps[i].tx;
		const int32_t ty0 = s_ctx.steps[i].ty;
		const float   x   = ox + TILE * (tx0 - s_ctx.tile_x);
		const float   y   = oy + TILE * (ty0 - s_ctx.tile_y);

		if (i != s_ctx.num_steps - 1) {
			const int32_t tx1 = s_ctx.steps[i + 1].tx;
			const int32_t ty1 = s_ctx.steps[i + 1].ty;
			path_render_arrow(tx0, ty0, tx1, ty1);
		}

		char buf[64];
		snprintf(buf, sizeof(buf), "%hhu", s_ctx.steps[i].price);
		const struct sprite_t* p = get_path_point(tx0, ty0);
		render_sprite(p, x + TILE * 0.25f, y + TILE * 0.25f);
		render_text(buf, x + TILE * 0.5f,  y + TILE * 0.5f - 1.0f, &TEXT);
	}
}

static void selector_render() {
	const float SELECTOR_OFFSET = -TILE * 0.5f;

	// TODO: @refactor Move it out into a common converter.
	const int32_t tile_x = s_ctx.selector_x - s_ctx.tile_x;
	const int32_t tile_y = s_ctx.selector_y - s_ctx.tile_y;

	// TODO: @refactor Move it out into a common check.
	if (!(tile_x >= 0 && tile_x < VIEW_TILES_PAD && tile_y >= 0 && tile_y < VIEW_TILES_PAD)) {
		return;
	}

	const float x = VIEW_OFFSET + s_ctx.map_x + tile_x * TILE + SELECTOR_OFFSET;
	const float y = VIEW_OFFSET + s_ctx.map_y + tile_y * TILE + SELECTOR_OFFSET;
	render_sprite(assets_sprites()->common.selector_location, x, y);
}

static int8_t dir(int32_t from, int32_t to) {
	const int32_t d = to - from;
	if (d >= 1) {
		return 1;
	}
	if (d <= -1) {
		return -1;
	}
	return 0;
}

static struct {
	int8_t  dx;
	int8_t  dy;
	uint8_t i;
} s_compass_lookup[] = {
	{ -1, -1, 0 },
	{  0, -1, 1 },
	{  1, -1, 2 },
	{ -1,  0, 3 },
	{  1,  0, 4 },
	{ -1,  1, 5 },
	{  0,  1, 6 },
	{  1,  1, 7 },
};

static uint8_t lookup_compass_sprite(int32_t px, int32_t py, int32_t cx, int32_t cy) {
	const size_t NUM_SPRITES = sizeof(s_compass_lookup) / sizeof(s_compass_lookup[0]);

	const int8_t dx = dir(cx, px);
	const int8_t dy = dir(cy, py);

	for (size_t i = 0; i < NUM_SPRITES; ++i) {
		if (s_compass_lookup[i].dx == dx && s_compass_lookup[i].dy == dy) {
			return s_compass_lookup[i].i;
		}
	}

	return 0;
}

static void compass_render() {
	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;
	const int32_t cx = s_ctx.tile_x + VIEW_TILES / 2;
	const int32_t cy = s_ctx.tile_y + VIEW_TILES / 2;

	const int32_t tx = px - s_ctx.tile_x;
	const int32_t ty = py - s_ctx.tile_y;

	bool is_player_visible = tx >= 0 && tx < VIEW_TILES && ty >= 0 && ty < VIEW_TILES;
	if (is_player_visible) return;

	const struct sprite_t* sprites[] = {
		assets_sprites()->travel_map.button_compass_nw,
		assets_sprites()->travel_map.button_compass_n,
		assets_sprites()->travel_map.button_compass_ne,
		assets_sprites()->travel_map.button_compass_w,
		assets_sprites()->travel_map.button_compass_e,
		assets_sprites()->travel_map.button_compass_sw,
		assets_sprites()->travel_map.button_compass_s,
		assets_sprites()->travel_map.button_compass_se,
	};
	const struct sprite_t* s = sprites[lookup_compass_sprite(px, py, cx, cy)];
	const float x = (SCREEN_SIZE - TILE) * 0.5f;
	const float y =  SCREEN_SIZE - TILE - TILE * 0.5f;
	// TODO: @robustness Use some constants for ids, unless imgui has a nice id handling.
	if (imgui_button(1, s, x, y, TILE, TILE)) {
		center_on_player();
	}
}

static void player_render() {
	const int32_t px = session_current()->player.x;
	const int32_t py = session_current()->player.y;

	if (!(px >= s_ctx.tile_x && px < s_ctx.tile_x + VIEW_TILES &&
	      py >= s_ctx.tile_y && py < s_ctx.tile_y + VIEW_TILES)) {
		return;
	}

	const int32_t tx = px - s_ctx.tile_x;
	const int32_t ty = py - s_ctx.tile_y;
	const float   x = VIEW_OFFSET + s_ctx.map_x + tx * TILE;
	const float   y = VIEW_OFFSET + s_ctx.map_y + ty * TILE;
	// TODO: Use a correct avatar.
	render_sprite(assets_sprites()->avatars.avatar_man2, x, y);
}

static void chrome_render() {
	render_sprite(assets_sprites()->travel_map.black_map_frame,    0.0f,  0.0f);
	render_sprite(assets_sprites()->travel_map.atlas_frame,       32.0f, 32.0f);

	// Planets influence
	render_sprite(assets_sprites()->travel_map.greek_letter_black_alpha,           0.0f, 0.0f);
	render_sprite(assets_sprites()->travel_map.greek_letter_black_omega, 512.0f - 32.0f, 0.0f);

	render_sprite(assets_sprites()->common.header,                32.0f,  0.0f);
	static const render_text_t title_params = {
		.font    = "regular",
		.size_pt = 28.0f,
		.align   = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE,
		.shadow  = true
	};
	render_text("Hello, sailor!", 512.0f * 0.5f, 16.0f, &title_params);
}

void states_travel_map_render(uint16_t width, uint16_t height, float dt) {
	if (!session_current()) return;

	// TODO: Render map view.
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);

	map_view_render();
	path_render();
	player_render();
	reveal_render();
	selector_render();
	chrome_render();
	compass_render();
	resources_render();
	coordinates_render(5 * 32.0f, 512.0f - 32.0f - 2.0f, s_ctx.selector_x, s_ctx.selector_y);
}
