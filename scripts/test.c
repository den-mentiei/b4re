struct sprite_t {
	const char* name;
	const char* texture_path;

	bgfx_texture_handle_t h;
};

struct {

	sprite_t sprites[];
} s_sprites = {

};
