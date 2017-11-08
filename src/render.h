#pragma once

#include <stdint.h>

// ABGR ? TODO: helpers.
typedef uint32_t color_t;

typedef struct {
	float x, y, z;
	float u, v;
	color_t color;
} __attribute__((packed)) render_vertex_t;

struct sprite_t;

void render_init();

/* inline color_t render_color(uint8_t r, uint8_t g, uint8_t b) { */
/* 	return (0xFF << 24) | (b << 16) | (g << 8) | r; */
/* } */

// TODO: ?
#define RENDER_COLOR(r, g, b) ((0xFF << 24) | (b << 16) | (g << 8) | r)

void render_sprite(const struct sprite_t* s, float x, float y, color_t color);

void render_shutdown();
