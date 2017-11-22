#pragma once

// TODO: @optimize Introduce macroblocks, update/track time/store status per block,
// rather per one location.

#define WORLD_PLANE_SIZE 256

typedef enum {
	TERRAIN_DEFAULT = 0,
	TERRAIN_ROCK_WATER,
	TERRAIN_ROCK_SOLID,
	TERRAIN_ROCK,
	TERRAIN_ROCK_SAND,
	TERRAIN_WILD,
	TERRAIN_GRASS,
	TERRAIN_EARTH,
	TERRAIN_CLAY,
	TERRAIN_SAND,
	TERRAIN_WATER,
	TERRAIN_WATER_BOTTOM,
	TERRAIN_WATER_DEEP,
} world_terrain_t;

typedef enum {
	TERRAIN_CLASS_DEFAULT = 0,
	TERRAIN_CLASS_ROCK,
	TERRAIN_CLASS_WILD,
	TERRAIN_CLASS_GRASS,
	TERRAIN_CLASS_EARTH,
	TERRAIN_CLASS_CLAY,
	TERRAIN_CLASS_SAND,
	TERRAIN_CLASS_WATER,
} world_terrain_class_t;

typedef struct {
	// TODO: @optimize Union?
	uint8_t terrain;
	bool    is_hidden;
	bool    has_data;
} world_location_t;

typedef struct {
	world_location_t locations[WORLD_PLANE_SIZE][WORLD_PLANE_SIZE];
} world_t;
