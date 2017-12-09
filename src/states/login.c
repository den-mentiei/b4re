#include "login.h"

#include "utils.h"
#include "imgui.h"
#include "log.h"
#include "render.h"
#include "game.h"
#include "session.h"
#include "generated/assets.h"

void states_login_update(uint16_t width, uint16_t height, float dt) {}

void states_login_render(uint16_t width, uint16_t height, float dt) {
	const struct {
		const char* name;
		const char* pass;
		const struct sprite_t* avatar;
	} users[] = {
		{ "amigo", "amigo_pass", assets_sprites()->avatars.avatar_man1 },
		{ "den",   "den_pass",   assets_sprites()->avatars.avatar_man2 },
		{ "jimon", "jimon_pass", assets_sprites()->avatars.avatar_man3 },
	};
	const size_t NUM_USERS     = ARRAY_SIZE(users);
	const float  AVATAR_SIZE   = 64.0f;
	const float  AVATAR_MARGIN = AVATAR_SIZE * 0.5f;

	float y = (height - AVATAR_SIZE) * 0.5f;
	float x = (width - (AVATAR_SIZE * NUM_USERS + AVATAR_MARGIN * (NUM_USERS - 1))) * 0.5f;

	for (size_t i = 0; i < NUM_USERS; ++i) {
		if (imgui_button(i + 1, users[i].avatar, x, y, AVATAR_SIZE, AVATAR_SIZE)) {
			session_start(users[i].name, users[i].pass);
			game_state_switch(GAME_STATE_LOADING);
		}

		x += AVATAR_SIZE + AVATAR_MARGIN;
	}
}
