#include "game.h"

#include <stdio.h>

#include "render.h"
#include "http.h"

#include "generated/assets.h"

#include "states/login.h"

/* static int state; */

/* static void test_handler(const uint8_t* data, size_t size, void* payload) { */
/* 	log_info((const char*)data); */
/* 	log_info("\n"); */
/* 	state++; */
/* } */

bool game_init(int32_t argc, const char* argv[]) {
	/* http_form_part_t form[] = { */
	/* 	{ "username", "den" }, */
	/* 	{ "password", "den_pass" } */
	/* }; */

	/* http_post_form("http://ancientlighthouse.com:8080/api/login", form, 2, test_handler, NULL); */

	/* while (state != 1) {}; */

	/* http_get("http://ancientlighthouse.com:8080/api/state", test_handler, NULL); */

	/* while (state != 2) {}; */

	/* http_post("http://ancientlighthouse.com:8080/api/logout", test_handler, NULL); */

	render_init();
	assets_init();

	return true;
}

bool game_update(uint16_t width, uint16_t height, float dt) {
	states_login_update(width, height, dt);
	return true;
}

void game_render(uint16_t width, uint16_t height, float dt) {
	states_login_render(width, height, dt);
}

void game_shutdown() {
	assets_shutdown();
	render_shutdown();
}
