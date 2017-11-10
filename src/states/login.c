#include "login.h"

#include "imgui.h"
#include "log.h"
#include "render.h"
#include "http.h"
#include "generated/assets.h"

/* static int state; */

/* static void test_handler(const uint8_t* data, size_t size, void* payload) { */
/* 	log_info((const char*)data); */
/* 	log_info("\n"); */
/* 	state++; */
/* } */

void states_login_update(uint16_t width, uint16_t height, float dt) {
	/* http_form_part_t form[] = { */
	/* 	{ "username", "den" }, */
	/* 	{ "password", "den_pass" } */
	/* }; */

	/* http_post_form("http://ancientlighthouse.com:8080/api/login", form, 2, test_handler, NULL); */

	/* while (state != 1) {}; */

	/* http_get("http://ancientlighthouse.com:8080/api/state", test_handler, NULL); */

	/* while (state != 2) {}; */

	/* http_post("http://ancientlighthouse.com:8080/api/logout", test_handler, NULL); */
}

void states_login_render(uint16_t width, uint16_t height, float dt) {
	struct {
		const char* name;
		const char* pass;
		const struct sprite_t* avatar;
	} users[] = {
		{ "amigo", "amigo_pass", assets_sprites()->avatars.avatar_man1 },
		{   "den",   "den_pass", assets_sprites()->avatars.avatar_man2 },
		{ "jimon", "jimon_pass", assets_sprites()->avatars.avatar_man3 },
	};
	const size_t NUM_USERS    = sizeof(users)/sizeof(users[0]);
	const float AVATAR_SIZE   = 64.0f;
	const float AVATAR_MARGIN = AVATAR_SIZE * 0.5f;

	float y = (height - AVATAR_SIZE) * 0.5f;
	float x = (width - (AVATAR_SIZE * NUM_USERS + AVATAR_MARGIN * (NUM_USERS - 1))) * 0.5f;

	for (size_t i = 0; i < NUM_USERS; ++i) {
		if (imgui_button(i + 1, users[i].avatar, x, y, AVATAR_SIZE, AVATAR_SIZE)) {
			log_info("[login] u=%s p=%s\n", users[i].name, users[i].pass);
		}

		x += AVATAR_SIZE + AVATAR_MARGIN;
	}
}
