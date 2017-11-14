#include "render_text.h"

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FONS_STATIC
#define FONTSTASH_IMPLEMENTATION
#include <fontstash.h>

#include "render.h"

#define MAX_FONTS 4
#define MAX_VERTICES FONS_VERTEX_COUNT

static struct {
	FONScontext* fons;

	int    loaded_fonts[MAX_FONTS];
	size_t free;

	render_tex_t tex;
	uint32_t     w, h;
	bool         has_texture;

	render_vertex_t vertices[MAX_VERTICES];
	uint16_t        indices[MAX_VERTICES];
} s_ctx;

static void delete_texture(void* uptr) {
	if (s_ctx.has_texture) {
		render_destroy_texture(s_ctx.tex);
		s_ctx.has_texture = false;
	}
}

static int resize_texture(void* uptr, int width, int height) {
	delete_texture(uptr);

	s_ctx.has_texture = true;
	s_ctx.tex         = render_create_texture_rgba8(width, height);
	s_ctx.w           = width;
	s_ctx.h           = height;

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

	uint32_t* pixels = (uint32_t*)render_update_texture_begin(w * h * 4);
	for (size_t j = 0; j < h; ++j) {
		for (size_t i = 0; i < w; ++i) {
			const uint8_t c = data[(j + y) * s_ctx.w + i + x];
			pixels[j * w + i] = (c << 24) | 0x00FFFFFF;
		}
	}

	render_update_texture_end(s_ctx.tex, x, y, w, h);
}

static void render(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) {
	if (!s_ctx.has_texture) return;
	
	render_vertex_t* vertices = s_ctx.vertices;
	for (size_t i = 0; i < nverts; ++i) {
		vertices[i].x = verts[i * 2 + 0];
		vertices[i].y = verts[i * 2 + 1];
		vertices[i].z = 0.0f;
		vertices[i].u = tcoords[i * 2 + 0];
		vertices[i].v = tcoords[i * 2 + 1];
		vertices[i].color = colors[i];
	}

	uint16_t* indices = s_ctx.indices;
	for (size_t i = 0; i < nverts / 3; ++i) {
		indices[i * 3 + 0] = (uint16_t)(i * 3 + 0);
		indices[i * 3 + 1] = (uint16_t)(i * 3 + 2);
		indices[i * 3 + 2] = (uint16_t)(i * 3 + 1);
	}

	render_transient(vertices, nverts, indices, nverts, s_ctx.tex);
}

void render_text_init() {
	const size_t ATLAS_WIDTH  = 1024;
	const size_t ATLAS_HEIGHT = 1024;

	FONSparams params;
	memset(&params, 0, sizeof(params));

	params.width  = ATLAS_WIDTH;
	params.height = ATLAS_HEIGHT;
	params.flags  = FONS_ZERO_TOPLEFT;

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

void render_text(const char* text, float x, float y, const render_text_t* params) {
	assert(text);
	assert(params);
	// TODO: Have a fallback font?
	assert(params->font);

	int f = fonsGetFontByName(s_ctx.fons, params->font);
	assert(f != FONS_INVALID);

	fonsClearState(s_ctx.fons);
	fonsSetAlign(s_ctx.fons, FONS_ALIGN_CENTER | FONS_ALIGN_BASELINE);
	fonsSetFont(s_ctx.fons,  f);

	if (params->size_pt != 0.0f)    fonsSetSize   (s_ctx.fons, params->size_pt);
	if (params->spacing_pt != 0.0f) fonsSetSpacing(s_ctx.fons, params->spacing_pt);
	if (params->color != 0)         fonsSetColor  (s_ctx.fons, params->color);
	if (params->align != 0)         fonsSetAlign  (s_ctx.fons, params->align);

	if (params->bounds_w || params->bounds_h) {
		float bounds[4];
		fonsTextBounds(s_ctx.fons, 0.0f, 0.0f, text, NULL, bounds);

		const int minx = bounds[0];
		const int miny = bounds[1];
		const int maxx = bounds[2];
		const int maxy = bounds[3];

		if (params->bounds_w) *params->bounds_w = maxx - minx;
		if (params->bounds_h) *params->bounds_h = maxy - miny;
	}

	if (params->shadow) {
		fonsPushState(s_ctx.fons);
		fonsSetBlur(s_ctx.fons,  0);
		fonsSetColor(s_ctx.fons, 0xFF000000);
		fonsDrawText(s_ctx.fons, x + 2.0f, y + 2.0f, text, NULL);
		fonsPopState(s_ctx.fons);
	}

	fonsDrawText(s_ctx.fons, x, y, text, NULL);
}
