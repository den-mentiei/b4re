#pragma once

#include <stdint.h>

typedef struct {
	float x, y, z;
	float u, v;
	uint32_t color; // ABGR ? TODO: helpers.
} __attribute__((packed)) render_vertex_t;

void render_init();

struct sprite_t;
void render_sprite(const struct sprite_t* s, float x, float y);

void render_shutdown();
