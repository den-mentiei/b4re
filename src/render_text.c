#include "render_text.h"

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <bgfx/bgfx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FONTSTASH_IMPLEMENTATION
#include <fontstash.h>

#include "render.h"

#define MAX_FONTS 4

static struct {
	FONScontext* fons;

	int    loaded_fonts[MAX_FONTS];
	size_t free;

	bgfx_texture_handle_t tex;
	uint32_t w, h;
	bool     has_texture;
} s_ctx;

static void delete_texture(void* uptr) {
	if (s_ctx.has_texture) {
		bgfx_destroy_texture(s_ctx.tex);
		s_ctx.has_texture = false;
	}
}

static int resize_texture(void* uptr, int width, int height) {
	delete_texture(uptr);

	s_ctx.tex = bgfx_create_texture_2d(width, height, false, 1, BGFX_TEXTURE_FORMAT_RGBA8, BGFX_TEXTURE_NONE, NULL);
	s_ctx.w = width;
	s_ctx.h = height;
	s_ctx.has_texture = true;

	return 1;
}

static int create_texture(void* uptr, int width, int height) {
	return resize_texture(uptr, width, height);
}

static void update_texture(void* uptr, int* rect, const unsigned char* data) {
	if (!s_ctx.has_texture) {
		return;
	}

	const uint32_t w = rect[2] - rect[0];
	const uint32_t h = rect[3] - rect[1];
	const uint32_t x = rect[0];
	const uint32_t y = rect[1];

	const bgfx_memory_t* mem = bgfx_alloc(w * h * 4);
	uint32_t* pixels = (uint32_t*)mem->data;
	for (size_t j = 0; j < h; ++j) {
		for (size_t i = 0; i < w; ++i) {
				const uint8_t c = data[(j + y) * s_ctx.w + i + x];
				pixels[j * w + i] = (c << 24) | 0x00FFFFFF;
			}
	}

	bgfx_update_texture_2d(s_ctx.tex, 0, 0, x, y, w, h, mem, -1);
}

static void render(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
	if (!s_ctx.has_texture) return;
	
	bgfx_transient_vertex_buffer_t tvb;
	bgfx_transient_index_buffer_t  tib;

	bgfx_alloc_transient_buffers(&tvb, render_vdecl(), nverts, &tib, nverts);

	render_vertex_t* vdata = (render_vertex_t*)tvb.data;
	for (size_t i = 0; i < nverts; ++i) {
		vdata[i].x = verts[i * 2 + 0];
		vdata[i].y = verts[i * 2 + 1];
		vdata[i].z = 0.0f;
		vdata[i].u = tcoords[i * 2 + 0];
		vdata[i].v = tcoords[i * 2 + 1];
		vdata[i].color = colors[i];
	}

	uint16_t* idata = (uint16_t*)tib.data;
	for (size_t i = 0; i < nverts / 3; ++i) {
		idata[i * 3 + 0] = (uint16_t)(i * 3 + 0);
		idata[i * 3 + 1] = (uint16_t)(i * 3 + 2);
		idata[i * 3 + 2] = (uint16_t)(i * 3 + 1);
	}

	render_transient(&tvb, nverts, &tib, nverts, s_ctx.tex.idx);
}

void render_text_init() {
	const size_t ATLAS_WIDTH  = 1024;
	const size_t ATLAS_HEIGHT = 1024;

	FONSparams params;
	memset(&params, 0, sizeof(params));

	params.width  = ATLAS_WIDTH;
	params.height = ATLAS_HEIGHT;
	params.flags  = (unsigned char)FONS_ZERO_TOPLEFT;

	params.renderCreate = create_texture;
	params.renderResize = resize_texture;
	params.renderUpdate = update_texture;
	params.renderDraw   = render;
	params.renderDelete = delete_texture;
	params.userPtr      = NULL;

	s_ctx.fons = fonsCreateInternal(&params);
	s_ctx.loaded_fonts[s_ctx.free++] = FONS_INVALID;
}

void render_text_shutdown() {
	fonsDeleteInternal(s_ctx.fons);
}

void render_load_font(const char* name, const char* path) {
	FILE* fp = fopen(path, "rb");
	if (!fp) return;

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0 ,SEEK_SET);

	if (!size)
	{
		fclose(fp);
		return;
	}

	uint8_t* data = (uint8_t*)malloc(size);
	fread(data, 1, size, fp);
	fclose(fp);

	fonsAddFontMem(s_ctx.fons, name, data, (int)size, 1);
}

void render_text(const char* text, const char* font, uint32_t color, float size_pt, float x, float y, bool shadow) {
	assert(text);

	int f = fonsGetFontByName(s_ctx.fons, font);
	assert(f != FONS_INVALID);

	fonsSetFont(s_ctx.fons,  f);
	fonsSetSize(s_ctx.fons,  size_pt);
	fonsSetBlur(s_ctx.fons,  0);
	fonsSetColor(s_ctx.fons, color);
	fonsSetAlign(s_ctx.fons, FONS_ALIGN_CENTER | FONS_ALIGN_MIDDLE);

	if (shadow) {
		fonsPushState(s_ctx.fons);
		fonsSetBlur(s_ctx.fons,  0);
		fonsSetColor(s_ctx.fons, 0xFF000000);
		fonsDrawText(s_ctx.fons, x + 2.0f, y + 2.0f, text, NULL);
		fonsPopState(s_ctx.fons);
	}

	fonsDrawText(s_ctx.fons, x, y, text, NULL);
}
