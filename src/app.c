#include "entry/entry.h"

#include <stdio.h>
#include <bgfx.h>
#include <platform.h>

#include "log.h"

static struct {
	uint16_t w;
	uint16_t h;
	uint32_t reset;
} ctx;

static void check_resized() {
	const entry_window_info_t* ewi = entry_get_window();
	if (ewi->width != ctx.w || ewi->height != ctx.h) {
		ctx.w = ewi->width;
		ctx.h = ewi->height;
		bgfx_reset(ctx.w, ctx.h, ctx.reset);
	}
}

#include <curl.h>

bool entry_init(int argc, const char* argv[]) {
	const entry_window_info_t* ewi = entry_get_window();
	ctx.w = ewi->width;
	ctx.h = ewi->height;
	ctx.reset = BGFX_RESET_VSYNC;

	bgfx_platform_data_t pd = {0};
	pd.ndt = ewi->display;
	pd.nwh = ewi->window;

	bgfx_set_platform_data(&pd);

	bgfx_init(BGFX_RENDERER_TYPE_COUNT, BGFX_PCI_ID_NONE, 0, NULL, NULL);
	bgfx_reset(ctx.w, ctx.h, ctx.reset);
	bgfx_set_debug(BGFX_DEBUG_TEXT);

	bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

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

bool entry_tick() {
	check_resized();

	bgfx_set_view_rect(0, 0, 0, ctx.w, ctx.h);
	bgfx_touch(0);

	bgfx_dbg_text_clear(0, false);
	bgfx_dbg_text_printf(0, 1, 0x4f, "b4re is alive and kicking");
	bgfx_dbg_text_printf(0, 2, 0x6f, "And even draws something!");

	bgfx_frame(false);

	return true;
}

void entry_shutdown() {
	bgfx_shutdown();
}
