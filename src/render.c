#include "render.h"

#include <bgfx/bgfx.h>

#include "shaders/tex_color_vs.h"
#include "shaders/tex_color_fs.h"

static struct {
	bgfx_vertex_decl_t    vdecl;
	bgfx_program_handle_t program;
	bgfx_uniform_handle_t u_texture;
} s_ctx = {0};

void render_init() {
	bgfx_vertex_decl_begin(&s_ctx.vdecl, BGFX_RENDERER_TYPE_NOOP);
		bgfx_vertex_decl_add(&s_ctx.vdecl, BGFX_ATTRIB_POSITION,  3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
		bgfx_vertex_decl_add(&s_ctx.vdecl, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, false, false);
		bgfx_vertex_decl_add(&s_ctx.vdecl, BGFX_ATTRIB_COLOR0,    4, BGFX_ATTRIB_TYPE_UINT8,  true, false);
	bgfx_vertex_decl_end(&s_ctx.vdecl);

	/* bgfx_shader_handle_t vs = bgfx_create_shader(bgfx_make_ref(tex_color_vs, sizeof(tex_color_vs))); */
	/* bgfx_shader_handle_t fs = bgfx_create_shader(bgfx_make_ref(tex_color_fs, sizeof(tex_color_fs))); */
	/* s_ctx.program = bgfx_create_program(vs, fs, true); */

	s_ctx.u_texture = bgfx_create_uniform("s_texture", BGFX_UNIFORM_TYPE_INT1, 1);
}

void render_shutdown() {

}
