#include "game.h"

#include <stdio.h>

#include "render.h"

#include "generated/assets.h"

#include "states/login.h"
#include "states/travel_map.h"

static struct {
	game_state_t state;
} s_ctx;

bool game_init(int32_t argc, const char* argv[]) {
	assets_init();
	return true;
}

bool game_update(uint16_t width, uint16_t height, float dt) {
	switch (s_ctx.state) {
		case GAME_STATE_LOGIN:
			states_login_update(width, height, dt);
			break;
		case GAME_STATE_MAP:
			states_travel_map_update(width, height, dt);
		default:
			break;
	}
	return true;
}

void game_render(uint16_t width, uint16_t height, float dt) {
	switch (s_ctx.state) {
		case GAME_STATE_LOGIN:
			states_login_render(width, height, dt);
			break;
		case GAME_STATE_MAP:
			states_travel_map_render(width, height, dt);
		default:
			break;
	}
}

void game_shutdown() {
	assets_shutdown();
}

void game_state_switch(game_state_t s) {
	s_ctx.state = s;
}
