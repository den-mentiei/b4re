#include "game.h"

#include <stdio.h>

#include "input.h"
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

	return true;
}

bool game_update(uint16_t width, uint16_t height, float dt) {
	float x, y;
	input_position(&x, &y);
	if (input_button_pressed(INPUT_BUTTON_LEFT)) {
		log_info("Left button clicked! %.2f %.2f\n", x, y);
	}
	if (input_button_pressed(INPUT_BUTTON_RIGHT)) {
		log_info("Right button clicked! %.2f %.2f\n", x, y);
	}
	return true;
}

void game_render(uint16_t width, uint16_t height, float dt) {
	const sprite_t* grass = &assets_sprites()->assets.travel_map.atlas_tiled_grass;
	render_sprite(grass, 32.0f, 32.0f, 448.0f, 448.0f);
}

void game_shutdown() {
	render_shutdown();
}
