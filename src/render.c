#include "render.h"

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

	static uint32_t white = 1;
	s_ctx.white_texture = bgfx_create_texture_2d(1, 1, false, 0, BGFX_TEXTURE_FORMAT_RGBA8, BGFX_TEXTURE_U_MIRROR | BGFX_TEXTURE_W_MIRROR | BGFX_TEXTURE_MAG_POINT | BGFX_TEXTURE_MIN_POINT, bgfx_make_ref(&white, sizeof(white)));
	bgfx_set_texture_name(s_ctx.white_texture, "dummy white texture");
}

void render_shutdown() {
	bgfx_destroy_program(s_ctx.program);
	bgfx_destroy_uniform(s_ctx.u_texture);
}
