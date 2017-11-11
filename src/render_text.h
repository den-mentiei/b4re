#pragma once

#include <stddef.h>
#include <stdbool.h>

void render_text_init();
void render_text_shutdown();

void render_load_font(const char* name, const char* path);
void render_text(const char* text, const char* font, float size_pt, float x, float y, bool shadow);
