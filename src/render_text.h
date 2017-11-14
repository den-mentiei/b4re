#pragma once

#include <stdint.h>
#include <stdbool.h>

void render_text_init();
void render_text_shutdown();

void render_load_font(const char* name, const char* path);
void render_text(const char* text, const char* font, uint32_t color, float size_pt, float x, float y, bool shadow);
void render_text_centered(const char* text, const char* font, uint32_t color, float size_pt, float x, float y, bool shadow);
