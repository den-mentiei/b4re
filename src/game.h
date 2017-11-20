#pragma once

#include <stdint.h>
#include <stdbool.h>

bool game_init(int32_t argc, const char* argv[]);
bool game_update(uint16_t width, uint16_t height, float dt);
void game_render(uint16_t width, uint16_t height, float dt);
void game_shutdown();

typedef enum {
	GAME_STATE_LOGIN = 0,
	GAME_STATE_LOADING,
	GAME_STATE_MAP
} game_state_t;
void game_state_switch(game_state_t s);
