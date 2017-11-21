#include "travel_map.h"

#include <assert.h>
#include <stdio.h> // snprintf

#include "game.h"
#include "log.h"
#include "session.h"
#include "imgui.h"
#include "input.h"
#include "render.h"
#include "render.inl"
#include "render_text.h"
#include "generated/assets.h"

static const size_t VIEW_TILES     = 7;
static const size_t VIEW_TILES_PAD = VIEW_TILES + 1;
static const size_t PLANE_TILES    = 256;
static const float  TILE           = 64.0f;
static const float  VIEW_SIZE      = VIEW_TILES * TILE;
static const float  VIEW_OFFSET    = 0.5f * TILE;
static const float  SCREEN_SIZE    = 8    * TILE;

static struct {
	float map_x;
	float map_y;
	int   tile_x;
	int   tile_y;

	int  selector_x;
	int  selector_y;
	bool has_selector;
} s_ctx;

static void update_scroll() {
	float x, y;
	input_position(&x, &y);

	float dx, dy;
	input_position_delta(&dx, &dy);

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

static void render_map_view() {
	const render_text_t DEBUG_TEXT = {
		.font    = "regular",
		.size_pt = 16.0f,
		.color   = render_color(255, 255, 255),
		.align   = RENDER_TEXT_ALIGN_CENTER | RENDER_TEXT_ALIGN_MIDDLE
	};

	char buf[64];

	const float ox = VIEW_OFFSET + s_ctx.map_x;
	const float oy = VIEW_OFFSET + s_ctx.map_y;

	bool can_select = !input_dragging(INPUT_BUTTON_LEFT);

	for (size_t i = 0; i < VIEW_TILES_PAD; ++i) {
		for (size_t j = 0; j < VIEW_TILES_PAD; ++j) {
			const size_t tx = s_ctx.tile_x + i;
			const size_t ty = s_ctx.tile_y + j;
			const float  x  = ox + TILE * i;
			const float  y  = oy + TILE * j;

			snprintf(buf, 64, "%zu,%zu", tx, ty);
			render_sprite(assets_sprites()->common.sign_green_dark, x, y);
			render_text(buf, x + TILE * 0.5f, y + TILE * 0.5f, &DEBUG_TEXT);

			if (can_select && imgui_button_invisible(j * VIEW_TILES_PAD + i + 1, x, y, TILE, TILE)) {
				s_ctx.selector_x = tx;
				s_ctx.selector_y = ty;
			}
		}
	}

	render_sprite(assets_sprites()->travel_map.mapnet, ox,          oy);
	render_sprite(assets_sprites()->travel_map.mapnet, ox,          oy + 256.0f);
	render_sprite(assets_sprites()->travel_map.mapnet, ox + 256.0f, oy);
	render_sprite(assets_sprites()->travel_map.mapnet, ox + 256.0f, oy + 256.0f);
}

static void center_on_player() {
	const uint32_t OFFSET_TO_CENTER = VIEW_TILES / 2;
	const uint32_t px = session_current()->player.x;
	const uint32_t py = session_current()->player.y;
	s_ctx.map_x       = 0.0f;
	s_ctx.map_y       = 0.0f;
	s_ctx.tile_x      = px - OFFSET_TO_CENTER;
	s_ctx.tile_y      = py - OFFSET_TO_CENTER;
	s_ctx.selector_x  = px;
	s_ctx.selector_y  = py;
}

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {
	assert(session_current());
	
	static bool was_map_requested;
	if (!was_map_requested) {
		was_map_requested = true;
		session_foo();
	}

	if (!s_ctx.has_selector) {
		center_on_player();
		s_ctx.has_selector = true;
	}

	if (input_dragging(INPUT_BUTTON_LEFT)) {
		update_scroll();
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

	int significant = snprintf(buf, 8, "%u", value);
	int num_zeros   = 3 - significant;
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

static void render_coordinates(float x, float y, uint8_t tiles_x, uint8_t tiles_y) {
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

	const char* we = tiles_x <= 127 ? "W" : "E";
	const char* ns = tiles_y <= 127 ? "N" : "S";
	const int   dx = tiles_x <= 127 ? 128 - tiles_x : tiles_x - 127;
	const int   dy = tiles_y <= 127 ? 128 - tiles_y : tiles_y - 127;

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

static void render_resources() {
	const indicator_rendering_t mind_rendering = {
		.zeros_color  = render_color(0x0D, 0x27, 0x34),
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
		.zeros_color  = render_color(0x2D, 0x07, 0x07),
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

static void render_movement() {
}

static void render_selector() {
	const float SELECTOR_OFFSET = -TILE * 0.5f;

	// TODO: @refactor Move it out into a common converter.
	const int32_t tile_x = s_ctx.selector_x - s_ctx.tile_x;
	const int32_t tile_y = s_ctx.selector_y - s_ctx.tile_y;

	// TODO: @refactor Move it out into a common check.
	if (!(tile_x >= 0 && tile_x < VIEW_TILES && tile_y >= 0 && tile_y < VIEW_TILES)) {
		return;
	}

	const float x = VIEW_OFFSET + s_ctx.map_x + tile_x * TILE + SELECTOR_OFFSET;
	const float y = VIEW_OFFSET + s_ctx.map_y + tile_y * TILE + SELECTOR_OFFSET;
	render_sprite(assets_sprites()->common.selector_location, x, y);
}

static int8_t dir(uint32_t from, uint32_t to) {
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

static uint8_t lookup_compass_sprite(uint32_t px, uint32_t py, uint32_t cx, uint32_t cy) {
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

static void render_compass() {
	const uint32_t px = session_current()->player.x;
	const uint32_t py = session_current()->player.y;
	const uint32_t cx = s_ctx.tile_x + VIEW_TILES / 2;
	const uint32_t cy = s_ctx.tile_y + VIEW_TILES / 2;

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
	render_sprite(s, x, y);

	if (imgui_button_invisible(1, x, y, TILE, TILE)) {
		center_on_player();
	}
}

static void render_player() {
	const uint32_t px = session_current()->player.x;
	const uint32_t py = session_current()->player.y;

	const uint32_t tx = px - s_ctx.tile_x;
	const uint32_t ty = py - s_ctx.tile_y;

	if (tx >= s_ctx.tile_x && tx < s_ctx.tile_x + VIEW_TILES &&
		ty >= s_ctx.tile_y && ty < s_ctx.tile_y + VIEW_TILES) {
		return;
	}

	const float x = VIEW_OFFSET + s_ctx.map_x + tx * TILE;
	const float y = VIEW_OFFSET + s_ctx.map_y + ty * TILE;
	render_sprite(assets_sprites()->avatars.avatar_man2, x, y);
}

static void render_chrome() {
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
	assert(session_current());

	// TODO: Render map view.
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);

	render_map_view();
	render_movement();
	render_player();
	render_selector();
	render_chrome();
	render_compass();
	render_resources();
	render_coordinates(5 * 32.0f, 512.0f - 32.0f, s_ctx.selector_x, s_ctx.selector_y);
}
