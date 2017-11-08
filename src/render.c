#include "render.h"

#include <string.h> // memcpy

#include <bgfx/bgfx.h>

#include "shaders/tex_color_vs.h"
#include "shaders/tex_color_fs.h"

#include "generated/assets.h"

static struct {
	bgfx_vertex_decl_t    vdecl;
	bgfx_program_handle_t program;
	bgfx_uniform_handle_t u_texture;
	bgfx_texture_handle_t white_texture;
} s_ctx = {0};

const uint64_t DEFAULT_STATE_2D =
	(0
	 | BGFX_STATE_RGB_WRITE
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
	s_ctx.white_texture = bgfx_create_texture_2d(1, 1, false, 0, BGFX_TEXTURE_FORMAT_RGBA8, BGFX_TEXTURE_U_MIRROR | BGFX_TEXTURE_W_MIRROR | BGFX_TEXTURE_MAG_POINT | BGFX_TEXTURE_MIN_POINT, bgfx_make_ref(&white, sizeof(white)));
	bgfx_set_texture_name(s_ctx.white_texture, "dummy white texture");

	assets_init();
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

void render_sprite(const sprite_t* s, float x, float y, color_t _color) {
	const int w = assets_sprites_width(s->index);
	const int h = assets_sprites_height(s->index);

#define VERT(i, _x, _y) s_vertices[i].x = _x; s_vertices[i].y = _y; s_vertices[i].color = _color;
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

	bgfx_texture_handle_t tex = assets_sprites_texture(s->index);

	bgfx_set_transient_vertex_buffer(0, &tvb, 0, NUM_VERTICES);
	bgfx_set_transient_index_buffer(&tib, 0, NUM_INDICES);
	bgfx_set_texture(0, s_ctx.u_texture, tex, UINT32_MAX);
	bgfx_set_state(DEFAULT_STATE_2D, 0);
	bgfx_submit(0, s_ctx.program, 0, false);
}

void render_shutdown() {
	assets_shutdown();

	bgfx_destroy_texture(s_ctx.white_texture);
	bgfx_destroy_uniform(s_ctx.u_texture);
	bgfx_destroy_program(s_ctx.program);
}
