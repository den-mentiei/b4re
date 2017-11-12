#pragma once

static inline color_t render_color(uint8_t r, uint8_t g, uint8_t b) {
	return (0xFF << 24) | (b << 16) | (g << 8) | r;
}
