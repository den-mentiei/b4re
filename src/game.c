#include "game.h"

#include <stdio.h>

#include "imgui.h"
#include "render.h"
#include "log.h"
#include "http.h"

#include "generated/assets.h"

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
	return true;
}

void game_render(uint16_t width, uint16_t height, float dt) {
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);

	const struct sprite_t* cs = assets_sprites()->avatars.avatar_man1;
	if (imgui_button(1, cs, 128.0f, 128.0f, 64.0f, 64.0f)) {
		log_info("man1 pressed.\n");
	}
}

void game_shutdown() {
	assets_shutdown();
	render_shutdown();
}
