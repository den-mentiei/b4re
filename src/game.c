#include "game.h"

#include <stdio.h>

#include "render.h"

#include "generated/assets.h"

#include "states/login.h"
#include "states/travel_map.h"

bool game_init(int32_t argc, const char* argv[]) {
	render_init();
	assets_init();
	return true;
}

bool game_update(uint16_t width, uint16_t height, float dt) {
	states_login_update(width, height, dt);
	/* states_travel_map_update(width, height, dt); */
	return true;
}

void game_render(uint16_t width, uint16_t height, float dt) {
	states_login_render(width, height, dt);
	/* states_travel_map_render(width, height, dt); */
}

void game_shutdown() {
	assets_shutdown();
	render_shutdown();
}
