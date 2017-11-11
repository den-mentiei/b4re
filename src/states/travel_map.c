#include "travel_map.h"

#include "game.h"
#include "session.h"
#include "input.h"
#include "render.h"
#include "generated/assets.h"

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {
	/* if (input_button_pressed(INPUT_BUTTON_LEFT)) { */
	/* 	if (session_current()->is_valid) { */
	/* 		session_end(); */
	/* 	} */
	/* 	game_state_switch(GAME_STATE_LOGIN); */
	/* } */
}

static void resource_indicator_render(float x, float y) {
	const size_t NUM_SEGMENTS = 12;

	// TODO: Render regen value text.
	render_sprite(assets_sprites()->travel_map.indicator_mind_regenerate_1s,   x, y);
	render_sprite(assets_sprites()->travel_map.indicator_mind_regenerate_time, x, y + 32.0f);

	y += 64.0f;
	for (size_t i = 0; i < NUM_SEGMENTS; ++i) {
		render_sprite(assets_sprites()->travel_map.indicator_mind_regenerate_full, x, y);
		y += 32.0f;
	}
}

static void resource_indicator_value_render(float x, float y, bool reversed) {
	const size_t NUM_BODY_SEGMENTS = 3;

	render_sprite(assets_sprites()->travel_map.indicator_mind_icon, x, y);

	const float dx = reversed ? -32.0f : 32.0f;
	x += dx;

	for (size_t i = 0; i < NUM_BODY_SEGMENTS; ++i) {
		render_sprite(assets_sprites()->travel_map.indicator_mind_body, x, y);
		x += dx;
	}
	
	render_sprite(assets_sprites()->travel_map.indicator_mind_edge, x, y);

	// TODO: Render values text.
}

static void coordinates_render(float x, float y) {
	render_sprite(assets_sprites()->travel_map.greek_letter_black_beta, x,         y);
	render_sprite(assets_sprites()->travel_map.dice_5p_brown,           x + 32.0f, y);

	// TODO: Render value text.
}

void states_travel_map_render(uint16_t width, uint16_t height, float dt) {
	// TODO: Render map view.
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);

	// Map chrome.
	render_sprite(assets_sprites()->travel_map.black_map_frame,    0.0f,  0.0f);
	render_sprite(assets_sprites()->travel_map.atlas_frame,       32.0f, 32.0f);
	render_sprite(assets_sprites()->common.header,                32.0f, 0.0);
	// TODO: Render title text.

	// Resources
	resource_indicator_render(0.0f,           32.0f);
	resource_indicator_render(512.0f - 32.0f, 32.0f);

	resource_indicator_value_render(0.0f,           512.0f - 32.0f, false);
	resource_indicator_value_render(512.0f - 32.0f, 512.0f - 32.0f, true);

	coordinates_render(5 * 32.0f, 512.0f - 32.0f);

	// Planets influence
	render_sprite(assets_sprites()->travel_map.greek_letter_black_alpha, 0.0f,           0.0f);
	render_sprite(assets_sprites()->travel_map.greek_letter_black_omega, 512.0f - 32.0f, 0.0f);

	render_sprite(assets_sprites()->travel_map.button_compass_n, 512.0f * 0.5f - 32.0f, 512.0f - 64.0f - 32.0f);
}
