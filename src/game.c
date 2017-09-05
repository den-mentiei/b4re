#include "game.h"

#include "log.h"
#include "http.h"

bool game_init(int32_t argc, const char* argv[]) {
	http_get("https://google.com");

	http_form_part_t form[] = {
		{ "username", "den" },
		{ "password", "den_pass" }
	};

	/* http_post_form("http://localhost:8080/api/login", form, 2); */
	http_post_form("http://ancientlighthouse.com:8080/api/login", form, 2);

	return true;
}

bool game_update(uint16_t width, uint16_t height, float dt) {
	return true;
}

void game_render(uint16_t width, uint16_t height, float dt) {

}

void game_shutdown() {

}
