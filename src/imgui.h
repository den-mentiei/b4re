#pragma once

#include <stdint.h>
#include <stdbool.h>

struct sprite_t;

void imgui_init();
void imgui_update();
void imgui_post_update();

bool imgui_button(int64_t id, const struct sprite_t* s, float x, float y, int w, int h);
