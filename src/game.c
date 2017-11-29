#include "game.h"

#include <stdio.h>

#include "render.h"
#include "render_text.h"
#include "allocator.h"

#include "client.h"
#include "session.h"
#include "states/login.h"
#include "states/loading.h"
#include "states/travel_map.h"

#include "generated/assets.h"

static struct {
	game_state_t state;
	game_state_t next_state;
} s_ctx;

bool game_init(int32_t argc, const char* argv[]) {
	assets_init();
	client_init();
	session_init(allocator_main());

	render_load_font("regular", "assets/fonts/Ancient_Lighthouse_Regular.otf");

	game_state_switch(GAME_STATE_LOADING);
	session_start("den", "den_pass");

	return true;
}

bool game_update(uint16_t width, uint16_t height, float dt) {
	client_update(dt);
	session_update(dt);

	s_ctx.state = s_ctx.next_state;

	switch (s_ctx.state) {
		case GAME_STATE_LOGIN:
			states_login_update(width, height, dt);
			break;
		case GAME_STATE_LOADING:
			states_loading_update(width, height, dt);
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
		case GAME_STATE_LOADING:
			states_loading_render(width, height, dt);
			break;
		case GAME_STATE_MAP:
			states_travel_map_render(width, height, dt);
		default:
			break;
	}
}

void game_shutdown() {
	session_shutdown();
	client_shutdown();
	assets_shutdown();
}

void game_state_switch(game_state_t s) {
	s_ctx.next_state = s;
}
