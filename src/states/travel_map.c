#include "travel_map.h"

#include "game.h"
#include "session.h"
#include "input.h"
#include "render.h"
#include "generated/assets.h"

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {
	if (input_button_pressed(INPUT_BUTTON_LEFT)) {
		session_end();
		game_state_switch(GAME_STATE_LOGIN);
	}
}

void states_travel_map_render(uint16_t width, uint16_t height, float dt) {
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);
}
