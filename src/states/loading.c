#include "loading.h"

#include "session.h"
#include "game.h"
#include "render.h"
#include "generated/assets.h"

void states_loading_update(uint16_t width, uint16_t height, float dt) {
	if (session_current()) {
		game_state_switch(GAME_STATE_MAP);
	}
}

void states_loading_render(uint16_t width, uint16_t height, float dt) {
	render_sprite(assets_sprites()->common.loading, 0.0f, 0.0f);
}
