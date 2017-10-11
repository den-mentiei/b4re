#ifdef VERTEX_SHADER

$input a_position, a_color0, a_texcoord0
$output v_color0, v_texcoord0

#include <bgfx/bgfx_shader.sh>

void main()
{
	mat4 projViewWorld = mul(mul(u_proj, u_view), u_model[0]); // TODO work it out after https://github.com/bkaradzic/bgfx/issues/983
	gl_Position = mul(projViewWorld, vec4(a_position, 1.0));
	v_color0 = a_color0;
	v_texcoord0 = a_texcoord0;
}

#endif

#ifdef FRAGMENT_SHADER

$input v_color0, v_texcoord0

#include <bgfx/bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

void main()
{
	gl_FragColor = vec4(texture2D(s_texture, v_texcoord0).rgba) * v_color0;
}

#endif