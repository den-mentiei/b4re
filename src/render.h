#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint32_t color_t;

typedef struct {
	float x, y, z;
	float u, v;
	color_t color;
} __attribute__((packed)) render_vertex_t;

void render_init();
void render_shutdown();

// TODO: @refactor Move it out - make loading code use low-level texture management code.
typedef size_t render_texture_t;
render_texture_t render_load_texture(const char* path);

const struct sprite_t* render_create_sprite(render_texture_t t, float u0, float v0, float u1, float v1);
void render_sprite(const struct sprite_t* s, float x, float y);
void render_sprite_colored(const struct sprite_t* s, float x, float y, color_t color);

// TODO: @refactor Move it out or change?
typedef struct {
	size_t tile_x;
	size_t tile_y;
	float  tile_w;
	float  tile_h;
} render_tile_t;
void render_tile(const struct sprite_t* tilemap, float x, float y, const render_tile_t* params);

// TODO: @robustness Static assert that it's enough.
typedef struct render_tex_t { uint64_t opaque; } render_tex_t;

render_tex_t render_create_texture_rgba8(uint32_t width, uint32_t height);
void render_destroy_texture(render_tex_t tex);

uint8_t* render_update_texture_begin(size_t size);
void render_update_texture_end(render_tex_t tex, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

void render_transient(const render_vertex_t* vertices, size_t num_vertices, const uint16_t* indices, size_t num_indices, render_tex_t tex);

