#include "world.h"

#include <stddef.h>
#include <assert.h>
#include <string.h> // memset
#include <math.h>	// floorf

#include "allocator.h"
#include "api.h"

#include "client.h"

// TODO: @optimize Store only up-to-date blocks as a hash-map over block coordinate.
// Then, store and update a non-used-time-interval to release blocks - LRU.

#define BLOCK_SIZE (1 << 4)
/* #define BLOCK_SIZE (1 << 5) */
#define NUM_BLOCKS (WORLD_PLANE_SIZE / BLOCK_SIZE)

#define BLOCK_LIFE_SPAN_SECONDS (30 * 1000.0f)

typedef struct {
	uint8_t type	  : 7;
	bool	is_hidden : 1;
} terrain_t;

typedef struct {
	terrain_t data[BLOCK_SIZE][BLOCK_SIZE];
} block_t;

typedef enum {
	BLOCK_STATE_NA = 0,
	BLOCK_STATE_NEEDED,
	BLOCK_STATE_REQUESTED,
	BLOCK_STATE_PRESENT
} block_state_t;

typedef struct world_t {
	struct allocator_t* alloc;

	// In seconds.
	float	age[NUM_BLOCKS][NUM_BLOCKS];
	block_t terrain[NUM_BLOCKS][NUM_BLOCKS];
	uint8_t state[NUM_BLOCKS][NUM_BLOCKS];
} world_t;

typedef struct {
	int8_t x;
	int8_t y;
	int8_t rx;
	int8_t ry;
} block_index_t;

static block_index_t to_block_index(int32_t x, int32_t y) {
	const int8_t bx = floorf((float)x / BLOCK_SIZE);
	const int8_t by = floorf((float)y / BLOCK_SIZE);
	const int8_t rx = x - bx * BLOCK_SIZE;
	const int8_t ry = y - by * BLOCK_SIZE;
	return (block_index_t) { .x = bx, .y = by, .rx = rx, .ry = ry };
}

static void block_requested(struct world_t* w, block_index_t bi) {
	assert(w);

	if (w->state[bi.x][bi.y] == BLOCK_STATE_NA) w->state[bi.x][bi.y] = BLOCK_STATE_NEEDED;
}

// PUBLIC API
// ==========

struct world_t* world_create(struct allocator_t* alloc) {
	assert(alloc);
	world_t* w = BR_ALLOC(alloc, sizeof(world_t));
	memset(w, 0, sizeof(world_t));
	w->alloc = alloc;
	return w;
}

void world_free(struct world_t* w) {
	assert(w);
	BR_FREE(w->alloc, w);
}

void world_update(struct world_t* w, float dt) {
	assert(w);
	
	for (size_t i = 0; i < NUM_BLOCKS; ++i) {
		for (size_t j = 0; j < NUM_BLOCKS; ++j) {
			switch (w->state[i][j]) {
				case BLOCK_STATE_NA:
					continue;

				case BLOCK_STATE_NEEDED:
					client_map(i * BLOCK_SIZE, j * BLOCK_SIZE, BLOCK_SIZE);
					w->state[i][j] = BLOCK_STATE_REQUESTED;
					break;

				// TODO:
				/* case BLOCK_STATE_PRESENT: */
				/*	w->age[i][j] += dt; */
				/*	if (w->age[i][j] > BLOCK_LIFE_SPAN_SECONDS) { */
				/*		w->age[i][j]   = 0.0f; */
				/*		w->state[i][j] = BLOCK_STATE_NA; */
				/*	} */
				/*	break; */
			}
		}
	}
}

void world_update_data(struct world_t* w, const struct api_map_t* map) {
	assert(w);
	assert(map);
	
	// TODO: @optimize Can calculate block spans and iterate per span.

	const size_t N = map->size;

	for (size_t i = 0; i < N; ++i) {
		for (size_t j = 0; j < N; ++j) {
			const int32_t tx = map->x + i;
			const int32_t ty = map->y + j;
			const block_index_t bi = to_block_index(tx, ty);
			
			w->state[bi.x][bi.y] = BLOCK_STATE_PRESENT;
			w->age[bi.x][bi.y]	 = 0.0f;

			const api_map_terrain_t* t = map->data + N * j + i;
			w->terrain[bi.x][bi.y].data[bi.rx][bi.ry].is_hidden = t->is_hidden;
			w->terrain[bi.x][bi.y].data[bi.rx][bi.ry].type		= t->type;
		}
	}
}

bool world_is_hidden(const struct world_t* w, int32_t x, int32_t y) {
	assert(w);
	assert(x >= 0 && x < WORLD_PLANE_SIZE);
	assert(y >= 0 && y < WORLD_PLANE_SIZE);
	
	const block_index_t bi = to_block_index(x, y);
	// TODO: Cast is a hack.
	block_requested((struct world_t*)w, bi);
	
	return w->state[bi.x][bi.y] != BLOCK_STATE_PRESENT || w->terrain[bi.x][bi.y].data[bi.rx][bi.ry].is_hidden;
}

uint8_t world_terrain(const struct world_t* w, int32_t x, int32_t y) {
	assert(w);
	assert(x >= 0 && x < WORLD_PLANE_SIZE);
	assert(y >= 0 && y < WORLD_PLANE_SIZE);
	assert(!world_is_hidden(w, x, y));
	
	const block_index_t bi = to_block_index(x, y);
	// TODO: Cast is a hack.
	block_requested((struct world_t*)w, bi);

	return w->state[bi.x][bi.y] == BLOCK_STATE_PRESENT ? w->terrain[bi.x][bi.y].data[bi.rx][bi.ry].type : 0;
}
