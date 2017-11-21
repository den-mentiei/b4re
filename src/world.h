#pragma once

#define WORLD_PLANE_SIZE 256

typedef struct {

} world_location_t;

typedef struct {
	world_location_t locations[WORLD_PLANE_SIZE][WORLD_PLANE_SIZE];
} world_t;

