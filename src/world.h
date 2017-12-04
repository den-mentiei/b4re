#pragma once

#define WORLD_PLANE_SIZE 256

// TODO: Move enums out.

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

// API SKETCH

struct allocator_t;
struct api_map_t;
struct world_t;

struct world_t* world_create(struct allocator_t* alloc);
void world_free(struct world_t* w);
void world_update(struct world_t* w, float dt);
void world_update_data(struct world_t* w, struct api_map_t* data);
