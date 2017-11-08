#include "travel_map.h"

#include "imgui.h"
#include "render.h"
#include "generated/assets.h"

void states_travel_map_update(uint16_t width, uint16_t height, float dt) {}

void states_travel_map_render(uint16_t width, uint16_t height, float dt) {
	render_sprite(assets_sprites()->travel_map.atlas_tiled_grass, 32.0f, 32.0f);
}
