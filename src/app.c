#include "entry/entry.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <gb_math.h>

#include "input.h"
#include "imgui.h"
#include "render.h"
#include "render_text.h"
#include "game.h"
#include "http.h"
#include "allocator.h"
#include "log.h"

static struct {
	uint16_t w;
	uint16_t h;
	uint32_t reset;
} s_ctx;

static void check_resized() {
	const entry_window_info_t* ewi = entry_get_window();
	if (ewi->width != s_ctx.w || ewi->height != s_ctx.h) {
		s_ctx.w = ewi->width;
		s_ctx.h = ewi->height;
		bgfx_reset(s_ctx.w, s_ctx.h, s_ctx.reset);
	}
}

bool entry_init(int32_t argc, const char* argv[]) {
	const entry_window_info_t* ewi = entry_get_window();
	s_ctx.w     = ewi->width;
	s_ctx.h     = ewi->height;
	s_ctx.reset = BGFX_RESET_VSYNC;

	bgfx_platform_data_t pd = {0};
	pd.ndt = ewi->display;
	pd.nwh = ewi->window;

	bgfx_set_platform_data(&pd);

	/* bgfx_init(BGFX_RENDERER_TYPE_METAL, BGFX_PCI_ID_NONE, 0, NULL, NULL); */
	bgfx_init(BGFX_RENDERER_TYPE_COUNT, BGFX_PCI_ID_NONE, 0, NULL, NULL);
	bgfx_reset(s_ctx.w, s_ctx.h, s_ctx.reset);
	bgfx_set_debug(BGFX_DEBUG_TEXT);

	bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

	render_init();
	render_text_init();
	http_init();
	imgui_init();

	return game_init(argc, argv);
}

bool entry_tick(float dt) {
	check_resized();

	bgfx_dbg_text_clear(0, false);
	
	input_update();
	imgui_update();
	bool should_continue = game_update(s_ctx.w, s_ctx.h, dt);

	bgfx_set_view_name(0, "main");
	bgfx_set_view_mode(0, BGFX_VIEW_MODE_SEQUENTIAL);

	bgfx_set_view_rect(0, 0, 0, s_ctx.w, s_ctx.h);
	bgfx_touch(0);
	gbMat4 proj;
	gb_mat4_ortho2d(&proj, 0.0f, s_ctx.w, s_ctx.h, 0.0f);
	bgfx_set_view_transform(0, NULL, proj.e);

	game_render(s_ctx.w, s_ctx.h, dt);

	// Should be called after anything that can call IMGUI.
	imgui_post_update();

	bgfx_frame(false);

	static bool requested;
	static bool got_response;
	static uint8_t buf[16 * 1024];
	static http_work_id_t id;
	if (!requested) {
		id = http_get("http://google.com", buf, sizeof(buf));
		requested = true;
	} else if (!got_response) {
		size_t got_bytes;
		if (http_status(id) == HTTP_STATUS_FINISHED && http_response_size(id, &got_bytes)) {
			log_info("!!! FINISHED and got %zu bytes", got_bytes);
			got_response = true;
		}
	}

	return should_continue;
}

void entry_shutdown() {
	game_shutdown();
	http_shutdown();
	render_text_shutdown();
	render_shutdown();
	bgfx_shutdown();
}
