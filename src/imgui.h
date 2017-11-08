#pragma once

#include <stdint.h>
#include <stdbool.h>

struct sprite_t;

void imgui_update();
void imgui_post_update();

// TODO: auto gen id? pass caption to gen id as in dear imgui?

bool imgui_button(int64_t id, const struct sprite_t* s, float x, float y, int w, int h);
