#include "game.h"

#include <curl.h>

#include "log.h"

bool game_init(int32_t argc, const char* argv[]) {
	CURLcode e = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (e != 0) {
		log_error("failed to init curl\n");
		return false;
	}

	const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
	log_info("curl version: %s\n", info->version);
	log_info("curl ssl version: %s\n", info->ssl_version);
	log_info("curl_ssl: %d\n", (info->features & CURL_VERSION_SSL) == 1);
	log_info("curl_zlib: %d\n", (info->features & CURL_VERSION_LIBZ) == 1);

	CURL* curl = curl_easy_init();
	if (!curl) {
		log_error("failed to init curl\n");
		return false;
	}

	curl_mime* mime = curl_mime_init(curl);
	curl_mimepart* part = curl_mime_addpart(mime);
	curl_mime_name(part, "username", CURL_ZERO_TERMINATED);
	curl_mime_data(part, "den", CURL_ZERO_TERMINATED);

	part = curl_mime_addpart(mime);
	curl_mime_name(part, "password", CURL_ZERO_TERMINATED);
	curl_mime_data(part, "den_pass", CURL_ZERO_TERMINATED);
 
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
	curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_URL, "http://ancientlighthouse.com:8080/api/login");
	/* curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/api/login"); */

	curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	curl_mime_free(mime);

	return true;
}

bool game_update(uint16_t width, uint16_t height, float dt) {

}

void game_render(uint16_t width, uint16_t height, float dt) {

}

void game_shutdown() {

}
