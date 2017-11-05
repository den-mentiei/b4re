#!/usr/bin/env python

import sys
import os
import pprint
import pystache

GROUP_DECL_TEMPLATE = r"""struct {
    const char* name;
    {{#sprites}}
    sprite_t {{name}};
    {{/sprites}}
    {{#groups}}
    {{> group_decl}}
    {{/groups}}
} {{name}};
"""

GROUP_LOAD_TEMPLATE = r""".{{name}} = {
    .name = "{{name}}",
    {{#sprites}}
    .{{name}} = { "{{name}}", {{index}} },
    {{/sprites}}
	{{#groups}}
    {{> group_load}}
	{{/groups}}
},
"""

HEADER_TEMPLATE = r"""#pragma once

// It is auto-generated :)

// TODO: remove it.
#include <bgfx/bgfx.h>

#include <stddef.h>

typedef struct sprite_t {
	const char* name;
    size_t      index;
} sprite_t;

typedef struct {
    {{#root.groups}}
	{{> group_decl}}
    {{/root.groups}}
} sprites_t;

void assets_init();
void assets_shutdown();

// TODO: remove it.
bgfx_texture_handle_t assets_sprites_texture(size_t i);

const sprites_t* assets_sprites();
"""

IMPLEMENTATION_TEMPLATE = r"""// It is auto-generated :)

#include "assets.h"

#include <stddef.h>
#include <stb_image.h>
#include <bgfx/bgfx.h>

static const char* s_pathes[{{path_count}}] = {
    {{#pathes}}
    "{{.}}",
    {{/pathes}}
};

static sprites_t s_sprites = {
{{#root.groups}}
	{{> group_load}}
{{/root.groups}}
};

static struct {
	bgfx_texture_handle_t handle;
    int w, h;
} s_textures[{{path_count}}] = {0};

void assets_init() {
	uint32_t flags = BGFX_TEXTURE_NONE
			| BGFX_TEXTURE_U_CLAMP | BGFX_TEXTURE_V_CLAMP
			| BGFX_TEXTURE_MAG_POINT | BGFX_TEXTURE_MIN_POINT;

    for (size_t i = 0; i < {{path_count}}; ++i) {
		int w = 0, h = 0, comp = 0;
		stbi_uc* bytes = stbi_load(s_pathes[i], &w, &h, &comp, 4);

		if (bytes) {
			// TODO: do we really need a copy here?
			s_textures[i].handle = bgfx_create_texture_2d(w, h, false, 1, BGFX_TEXTURE_FORMAT_RGBA8, flags, bgfx_copy(bytes, w * h * 4));
			s_textures[i].w      = w;
			s_textures[i].h      = h;
			stbi_image_free(bytes);
		}
    }
}

const sprites_t* assets_sprites() {
    return &s_sprites;
}

bgfx_texture_handle_t assets_sprites_texture(size_t i) {
    return s_textures[i].handle;
}

void assets_shutdown() {
    for (size_t i = 0; i < {{path_count}}; ++i) {
		bgfx_destroy_texture(s_textures[i].handle);
    }
}
"""

PARTIALS = {
	'group_decl': GROUP_DECL_TEMPLATE,
	'group_load': GROUP_LOAD_TEMPLATE,
}

def split_path(path):
	return os.path.normpath(path).split(os.path.sep)

def make_empty_group():
	return { 'children': {}, 'files': [] }

def build_subtree(d, parts):
	for p in parts:
		if p not in d['children']:
			d['children'][p] = make_empty_group()
		d = d['children'][p]
	return d

def is_sprite_texture(name):
	return os.path.splitext(name)[1] == '.png'

def traverse_assets(root, is_asset):
	assets = make_empty_group()
	for (path, dirs, files) in os.walk(root):
		s = build_subtree(assets, split_path(path))
		s['files'] = [os.path.join(path, f) for f in files if is_asset(f)]
	return assets

def make_sprite(f):
	name = os.path.splitext(os.path.basename(f))[0].replace('-', '_')
	return {'name': name, 'path': f}

def make_group(name, leaf):
	sprites = [make_sprite(f) for f in leaf['files']]
	nested  = [make_group(k, v) for (k, v) in leaf['children'].items()]
	return {
		'name':    name,
		'groups':  nested,
		'sprites': sprites
	}

def build_groups(assets):
	return make_group('root', assets)

def update_sprites(root, f):
	root['sprites'] = [f(s) for s in root['sprites']]
	root['groups']  = [update_sprites(g, f) for g in root['groups']]
	return root

def make_index_gatherer(pathes):
	def gather_and_set_index(s):
		nonlocal pathes
		i = len(pathes)
		pathes.append(s['path'])
		return { 'name': s['name'], 'index': i }
	return gather_and_set_index

def dump(x):
	p = pprint.PrettyPrinter(indent=2)
	p.pprint(x)

def main():
	if len(sys.argv) != 3:
		print('Usage: ./sprites.py <assets_dir> <output_dir>')
		exit(1)

	assets  = sys.argv[1]
	output  = sys.argv[2]
	header  = os.path.join(output, 'assets.h')
	code    = os.path.join(output, 'assets.c')
	groups  = build_groups(traverse_assets(assets, is_sprite_texture))
	ps      = []
	indexed = update_sprites(groups, make_index_gatherer(ps))

	data = {
		'root':       indexed,
		'pathes':     ps,
		'path_count': len(ps)
	}
	# dump(data)

	r = pystache.Renderer(partials=PARTIALS)
	with open(header, 'w') as f:
		f.write(r.render(HEADER_TEMPLATE, data))
	with open(code, 'w') as f:
		f.write(r.render(IMPLEMENTATION_TEMPLATE, data))

if __name__ == '__main__':
	main()
