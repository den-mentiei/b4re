#include "render.h"

#include <assert.h>
#include <string.h> // memcpy
#include <math.h>   // floor

#include <stb_image.h>
#include <bgfx/bgfx.h>

#include "shaders/tex_color_vs.h"
#include "shaders/tex_color_fs.h"

// TODO: ?
#define MAX_TEXTURES 1024
#define MAX_SPRITES  1024

typedef struct sprite_t {
	render_texture_t tex;
	float w,  h;
	float u0, v0;
	float u1, v1;
} sprite_t;

static struct {
	bgfx_vertex_decl_t    vdecl;
	bgfx_program_handle_t program;
	bgfx_uniform_handle_t u_texture;

	const bgfx_memory_t* pending_memory;

	struct {
		size_t count;
		struct {
			bgfx_texture_handle_t handle;
			float w, h;
		} data[MAX_TEXTURES];
	} textures;

	struct {
		size_t   count;
		sprite_t data[MAX_SPRITES];
	} sprites;
} s_ctx = {0};

const uint64_t DEFAULT_STATE_2D =
	(0
	 | BGFX_STATE_RGB_WRITE
	 | BGFX_STATE_BLEND_ALPHA
	 | BGFX_STATE_ALPHA_WRITE
	 | BGFX_STATE_CULL_CCW
	 );

void render_init() {
	bgfx_vertex_decl_begin(&s_ctx.vdecl, BGFX_RENDERER_TYPE_NOOP);
		bgfx_vertex_decl_add(&s_ctx.vdecl, BGFX_ATTRIB_POSITION,  3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
		bgfx_vertex_decl_add(&s_ctx.vdecl, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
		bgfx_vertex_decl_add(&s_ctx.vdecl, BGFX_ATTRIB_COLOR0,    4, BGFX_ATTRIB_TYPE_UINT8,  true, false);
	bgfx_vertex_decl_end(&s_ctx.vdecl);

	bgfx_shader_handle_t vs = bgfx_create_shader(bgfx_make_ref(tex_color_vs, sizeof(tex_color_vs)));
	bgfx_set_shader_name(vs, "tex_color_vs");
	bgfx_shader_handle_t fs = bgfx_create_shader(bgfx_make_ref(tex_color_fs, sizeof(tex_color_fs)));
	bgfx_set_shader_name(fs, "tex_color_fs");
	s_ctx.program = bgfx_create_program(vs, fs, true);

	s_ctx.u_texture = bgfx_create_uniform("s_texture", BGFX_UNIFORM_TYPE_INT1, 1);

	static uint32_t white = 0xFFFFFFFF;
	bgfx_texture_handle_t white_texture = bgfx_create_texture_2d(1, 1, false, 0, BGFX_TEXTURE_FORMAT_RGBA8, BGFX_TEXTURE_U_MIRROR | BGFX_TEXTURE_W_MIRROR | BGFX_TEXTURE_MAG_POINT | BGFX_TEXTURE_MIN_POINT, bgfx_make_ref(&white, sizeof(white)));
	bgfx_set_texture_name(white_texture, "dummy white texture");

	render_texture_t white_id            = s_ctx.textures.count++;
	s_ctx.textures.data[white_id].handle = white_texture;
	s_ctx.textures.data[white_id].w      = 1;
	s_ctx.textures.data[white_id].h      = 1;
}

void render_shutdown() {
	for (size_t i = 0; i < s_ctx.textures.count; ++i) {
		bgfx_destroy_texture(s_ctx.textures.data[i].handle);
	}

	bgfx_destroy_uniform(s_ctx.u_texture);
	bgfx_destroy_program(s_ctx.program);
}

render_texture_t render_load_texture(const char* path) {
	assert(s_ctx.textures.count < MAX_TEXTURES);

	render_texture_t id = s_ctx.textures.count++;

	const uint32_t flags = BGFX_TEXTURE_NONE
			| BGFX_TEXTURE_U_CLAMP | BGFX_TEXTURE_V_CLAMP
			| BGFX_TEXTURE_MAG_POINT | BGFX_TEXTURE_MIN_POINT;

	int w = 0, h = 0, comp = 0;
	stbi_uc* bytes = stbi_load(path, &w, &h, &comp, 4);

	if (bytes) {
		// TODO: do we really need a copy here?
		bgfx_texture_handle_t handle = bgfx_create_texture_2d(w, h, false, 1, BGFX_TEXTURE_FORMAT_RGBA8, flags, bgfx_copy(bytes, w * h * 4));
		s_ctx.textures.data[id].handle = handle;
		s_ctx.textures.data[id].w      = w;
		s_ctx.textures.data[id].h      = h;
		bgfx_set_texture_name(handle, path);
		stbi_image_free(bytes);
	} else {
		id = 0;
	}

	return id;
}

const sprite_t* render_create_sprite(render_texture_t t, float u0, float v0, float u1, float v1) {
	assert(s_ctx.sprites.count < MAX_SPRITES);
	assert(t < s_ctx.textures.count);

	sprite_t* s = &s_ctx.sprites.data[s_ctx.sprites.count++];

	const float w = s_ctx.textures.data[t].w;
	const float h = s_ctx.textures.data[t].h;
	
	s->tex = t;
	s->w   = w;
	s->h   = h;
	s->u0  = u0;
	s->v0  = v0;
	s->u1  = u1;
	s->v1  = v1;

	return s;
}

static render_vertex_t s_vertices[] = {
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
	{ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF },
	{ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF },
	{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF },
};

// 0 1
// 3 2
static uint16_t s_indices[] = {
	0, 1, 2,
	2, 3, 0
};

void render_sprite(const sprite_t* s, float x, float y) {
	render_sprite_colored(s, x, y, 0xFFFFFFFF);
}

void render_sprite_colored(const sprite_t* s, float x, float y, color_t color) {
	const float w = s->w;
	const float h = s->h;

#define VERT(i, _x, _y) s_vertices[i].x = _x; s_vertices[i].y = _y; s_vertices[i].color = color;
	VERT(0, x,     y);
	VERT(1, x + w, y);
	VERT(2, x + w, y + h);
	VERT(3, x,     y + h);
#undef VERT

	const size_t NUM_INDICES =  sizeof(s_indices) / sizeof(s_indices[0]);
	const size_t NUM_VERTICES = sizeof(s_vertices) / sizeof(s_vertices[0]);

	bgfx_transient_vertex_buffer_t tvb;
	bgfx_transient_index_buffer_t  tib;

	bgfx_alloc_transient_buffers(&tvb, &s_ctx.vdecl, NUM_VERTICES, &tib, NUM_INDICES);

	memcpy(tvb.data, s_vertices, tvb.size);
	memcpy(tib.data, s_indices, tib.size);

	bgfx_texture_handle_t tex = s_ctx.textures.data[s->tex].handle;

	bgfx_set_transient_vertex_buffer(0, &tvb, 0, NUM_VERTICES);
	bgfx_set_transient_index_buffer(&tib, 0, NUM_INDICES);
	bgfx_set_texture(0, s_ctx.u_texture, tex, UINT32_MAX);
	bgfx_set_state(DEFAULT_STATE_2D, 0);
	bgfx_submit(0, s_ctx.program, 0, false);
}

render_tex_t render_create_texture_rgba8(uint32_t width, uint32_t height) {
	render_tex_t result;
	bgfx_texture_handle_t tex = bgfx_create_texture_2d(width, height, false, 1, BGFX_TEXTURE_FORMAT_RGBA8, BGFX_TEXTURE_NONE, NULL);
	memcpy(&result, &tex, sizeof(tex));
	return result;
}

void render_destroy_texture(render_tex_t tex) {
	bgfx_texture_handle_t th;
	memcpy(&th, &tex, sizeof(th));
	bgfx_destroy_texture(th);
}

uint8_t* render_update_texture_begin(size_t size) {
	assert(!s_ctx.pending_memory);

	const bgfx_memory_t* mem = bgfx_alloc(size); 
	s_ctx.pending_memory     = mem;
	return mem->data;
}

void render_update_texture_end(render_tex_t tex, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
	assert(s_ctx.pending_memory);

	bgfx_texture_handle_t th;
	memcpy(&th, &tex, sizeof(th));

	bgfx_update_texture_2d(th, 0, 0, x, y, w, h, s_ctx.pending_memory, -1);
	s_ctx.pending_memory = NULL;
}

void render_transient(const render_vertex_t* vertices, size_t num_vertices, const uint16_t* indices, size_t num_indices, render_tex_t tex) {
	assert(vertices && num_vertices > 0);
	assert(indices && num_indices > 0);

	bgfx_transient_vertex_buffer_t tvb;
	bgfx_transient_index_buffer_t  tib;

	bgfx_alloc_transient_buffers(&tvb, &s_ctx.vdecl, num_vertices, &tib, num_indices);

	memcpy(tvb.data, vertices, tvb.size);
	memcpy(tib.data, indices,  tib.size);

	bgfx_texture_handle_t th;
	memcpy(&th, &tex, sizeof(th));

	bgfx_set_transient_vertex_buffer(0, &tvb, 0, num_vertices);
	bgfx_set_transient_index_buffer(&tib, 0, num_indices);
	bgfx_set_texture(0, s_ctx.u_texture, th, UINT32_MAX);
	bgfx_set_state(DEFAULT_STATE_2D, 0);
	bgfx_submit(0, s_ctx.program, 0, false);
}

static render_vertex_t s_tile_vertices[] = {
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
	{ 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0xFFFFFFFF },
	{ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF },
	{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF },
};

void render_tile(const struct sprite_t* tilemap, float x, float y, const render_tile_t* params) {
	assert(tilemap);
	assert(params);
	assert(params->tile_w > 0);
	assert(params->tile_h > 0);

	const float  w = params->tile_w;
	const float  h = params->tile_h;
	const size_t NUM_TILES_X = floor(tilemap->w / params->tile_w);
	const size_t NUM_TILES_Y = floor(tilemap->h / params->tile_h);

	// TODO: Hard-coded for non-atlassed sprites, assumes 0..1 uvs.
	const float du = 1.0f / NUM_TILES_X;
	const float dv = 1.0f / NUM_TILES_Y;
	const float u0 = params->tile_x * du;
	const float v0 = params->tile_y * dv;
	const float u1 = u0 + du;
	const float v1 = v0 + dv;

	assert(params->tile_x < NUM_TILES_X);
	assert(params->tile_y < NUM_TILES_Y);

#define VERT(i, _x, _y, _u, _v)            \
	s_tile_vertices[i].x = _x;             \
	s_tile_vertices[i].y = _y;             \
	s_tile_vertices[i].u = _u;             \
	s_tile_vertices[i].v = _v;             \
	s_tile_vertices[i].color = 0xFFFFFFFF;

	VERT(0, x,     y,     u0, v0);
	VERT(1, x + w, y,     u1, v0);
	VERT(2, x + w, y + h, u1, v1);
	VERT(3, x,     y + h, u0, v1);

#undef VERT

	const size_t NUM_INDICES =  sizeof(s_indices) / sizeof(s_indices[0]);
	const size_t NUM_VERTICES = sizeof(s_tile_vertices) / sizeof(s_tile_vertices[0]);

	bgfx_transient_vertex_buffer_t tvb;
	bgfx_transient_index_buffer_t  tib;

	bgfx_alloc_transient_buffers(&tvb, &s_ctx.vdecl, NUM_VERTICES, &tib, NUM_INDICES);

	memcpy(tvb.data, s_tile_vertices, tvb.size);
	memcpy(tib.data, s_indices, tib.size);

	bgfx_texture_handle_t tex = s_ctx.textures.data[tilemap->tex].handle;

	bgfx_set_transient_vertex_buffer(0, &tvb, 0, NUM_VERTICES);
	bgfx_set_transient_index_buffer(&tib, 0, NUM_INDICES);
	bgfx_set_texture(0, s_ctx.u_texture, tex, UINT32_MAX);
	bgfx_set_state(DEFAULT_STATE_2D, 0);
	bgfx_submit(0, s_ctx.program, 0, false);
}
