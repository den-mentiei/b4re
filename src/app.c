#include "entry/entry.h"

#include <stdio.h>
#include <bgfx.h>
#include <platform.h>

#include "log.h"
#include "game.h"

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

bool entry_init(int32_t argc, const char* argv[]) {
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

	return game_init(argc, argv);
}

bool entry_tick() {
	check_resized();

	bgfx_set_view_rect(0, 0, 0, ctx.w, ctx.h);
	bgfx_touch(0);

	bgfx_dbg_text_clear(0, false);
	bgfx_dbg_text_printf(0, 0, 0x4f, "b4re is alive and kicking");

	bgfx_frame(false);

	float dt = 1/60.0f;
	return game_update(ctx.w, ctx.h, dt);
}

void entry_shutdown() {
	game_shutdown();
	bgfx_shutdown();
}
