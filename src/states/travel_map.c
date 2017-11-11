#include "travel_map.h"

#include "game.h"
#include "session.h"
#include "input.h"
#include "render.h"
#include "render_text.h"
#include "generated/assets.h"

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {
	/* if (input_button_pressed(INPUT_BUTTON_LEFT)) { */
	/* 	if (session_current()->is_valid) { */
	/* 		session_end(); */
	/* 	} */
	/* 	game_state_switch(GAME_STATE_LOGIN); */
	/* } */
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
} indicator_sprites_t;

static void resource_indicator_render(const indicator_sprites_t* sprites, float x, float y) {
	const size_t NUM_SEGMENTS = 12;

	// TODO: Render regen value text.
	render_sprite(sprites->regen[0], x, y);
	render_sprite(sprites->time,     x, y + 32.0f);

	y += 64.0f;
	for (size_t i = 0; i < NUM_SEGMENTS; ++i) {
		render_sprite(sprites->segment_full, x, y);
		y += 32.0f;
	}
}

static void resource_indicator_value_render(const indicator_sprites_t* sprites, float x, float y, bool reversed) {
	const size_t NUM_BODY_SEGMENTS = 3;

	render_sprite(sprites->icon, x, y);

	const float dx = reversed ? -32.0f : 32.0f;
	x += dx;

	for (size_t i = 0; i < NUM_BODY_SEGMENTS; ++i) {
		render_sprite(sprites->body, x, y);
		x += dx;
	}
	
	render_sprite(sprites->edge, x, y);

	// TODO: Render values text.
}

static void coordinates_render(float x, float y) {
	render_sprite(assets_sprites()->travel_map.greek_letter_black_beta, x,         y);
	render_sprite(assets_sprites()->travel_map.dice_5p_brown,           x + 32.0f, y);

	// TODO: Render value text.
}

static void resources_render() {
	const indicator_sprites_t mind_sprites = {
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
	const indicator_sprites_t matter_sprites = {
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
	resource_indicator_render(&mind_sprites,   0.0f,           32.0f);
	resource_indicator_render(&matter_sprites, 512.0f - 32.0f, 32.0f);

	resource_indicator_value_render(&mind_sprites,   0.0f,           512.0f - 32.0f, false);
	resource_indicator_value_render(&matter_sprites, 512.0f - 32.0f, 512.0f - 32.0f, true);
}

void states_travel_map_render(uint16_t width, uint16_t height, float dt) {
	// TODO: Render map view.
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);

	// Map chrome.
	render_sprite(assets_sprites()->travel_map.black_map_frame,    0.0f,  0.0f);
	render_sprite(assets_sprites()->travel_map.atlas_frame,       32.0f, 32.0f);
	render_sprite(assets_sprites()->common.header,                32.0f, 0.0);
	// TODO: Render title text.

	resources_render();
	coordinates_render(5 * 32.0f, 512.0f - 32.0f);

	// Planets influence
	render_sprite(assets_sprites()->travel_map.greek_letter_black_alpha, 0.0f,           0.0f);
	render_sprite(assets_sprites()->travel_map.greek_letter_black_omega, 512.0f - 32.0f, 0.0f);

	render_sprite(assets_sprites()->travel_map.button_compass_n, 512.0f * 0.5f - 32.0f, 512.0f - 64.0f - 32.0f);
	render_text("Hello, sailor!", "regular", 24.0f, 64.0f, 20.0f);
}
