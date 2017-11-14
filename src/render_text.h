#pragma once

#include <stdint.h>
#include <stdbool.h>

void render_text_init();
void render_text_shutdown();

typedef enum render_text_align_t {
	RENDER_TEXT_ALIGN_DEFAULT  = 0,
	RENDER_TEXT_ALIGN_LEFT     = 1 << 0,
	// Horizontal align default.
	RENDER_TEXT_ALIGN_CENTER   = 1 << 1,
	RENDER_TEXT_ALIGN_RIGHT    = 1 << 2,
	RENDER_TEXT_ALIGN_TOP      = 1 << 3,
	RENDER_TEXT_ALIGN_MIDDLE   = 1 << 4,
	RENDER_TEXT_ALIGN_BOTTOM   = 1 << 5,
	// Vertical align default.
	RENDER_TEXT_ALIGN_BASELINE = 1 << 6
} render_text_align_t;

typedef struct render_text_t {
	const char* font;

	float size_pt;
	float spacing_pt;

	// Both can be NULL - optional.
	float* bounds_w;
	float* bounds_h;

	uint32_t color;

	render_text_align_t align;
	bool shadow;
} render_text_t;

void render_load_font(const char* name, const char* path);
void render_text(const char* text, float x, float y, const render_text_t* params);
