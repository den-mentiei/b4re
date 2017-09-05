#pragma once

#include <stdint.h>
#include <stdbool.h>

bool game_init(int32_t argc, const char* argv[]);
bool game_update(uint16_t width, uint16_t height, float dt);
void game_render(uint16_t width, uint16_t height, float dt);
void game_shutdown();
