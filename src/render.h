#pragma once

#include <stdint.h>
#include <stddef.h>

// ABGR ? TODO: helpers.
typedef uint32_t color_t;

typedef struct {
	float x, y, z;
	float u, v;
	color_t color;
} __attribute__((packed)) render_vertex_t;

typedef size_t render_texture_t;
struct sprite_t;

/* inline color_t render_color(uint8_t r, uint8_t g, uint8_t b) { */
/* 	return (0xFF << 24) | (b << 16) | (g << 8) | r; */
/* } */

// TODO: ?
#define RENDER_COLOR(r, g, b) ((0xFF << 24) | (b << 16) | (g << 8) | r)

void render_init();
void render_shutdown();

render_texture_t render_load_texture(const char* path);
const struct sprite_t* render_create_sprite(render_texture_t t, float u0, float v0, float u1, float v1);
void render_sprite(const struct sprite_t* s, float x, float y);
void render_sprite_colored(const struct sprite_t* s, float x, float y, color_t color);
