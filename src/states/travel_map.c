#include "travel_map.h"

#include <assert.h>
#include <stdio.h> // snprintf

#include "game.h"
#include "session.h"
#include "input.h"
#include "render.h"
#include "render.inl"
#include "render_text.h"
#include "generated/assets.h"

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {
	if (input_button_pressed(INPUT_BUTTON_LEFT)) {
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
	snprintf(buf, 8, "%0*d", zeros, 0);
	text_params.color = params->color;
	render_text(buf, x, y, &text_params);
	x += w;

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

static void coordinates_render(float x, float y) {
	render_sprite(assets_sprites()->travel_map.greek_letter_black_beta, x,         y);
	render_sprite(assets_sprites()->travel_map.dice_5p_brown,           x + 32.0f, y);

	// TODO: Render value text.
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
	render_sprite(assets_sprites()->avatars.avatar_man2, 32.0f + 64.0f * x, 32.0f + 64.0f * y);
}

void states_travel_map_render(uint16_t width, uint16_t height, float dt) {
	if (!session_current()) return;

	// TODO: Render map view.
	render_sprite(assets_sprites()->travel_map.atlas_tiled_warfog, 32.0f, 32.0f);
	render_movement();

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
	coordinates_render(5 * 32.0f, 512.0f - 32.0f);
}
