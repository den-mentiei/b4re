#include "world.h"

#include <stddef.h>
#include <assert.h>
#include <string.h> // memset
#include <math.h>   // floorf

#include "allocator.h"
#include "api.h"

// TODO: @optimize Store only up-to-date blocks as a hash-map over block coordinate.

#define BLOCK_SIZE (1 << 5)
#define NUM_BLOCKS (WORLD_PLANE_SIZE / BLOCK_SIZE)

#define BLOCK_LIFE_SPAN_SECONDS 30.0f

typedef struct {
    uint8_t type      : 7;
    bool    is_hidden : 1;
} terrain_t;

typedef struct world_t {
    struct allocator_t* alloc;

    // In seconds.
    float     age[NUM_BLOCKS][NUM_BLOCKS];
    terrain_t terrain[NUM_BLOCKS][NUM_BLOCKS];
    bool      has_data[NUM_BLOCKS][NUM_BLOCKS];
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

// PUBLIC API
// ==========

struct world_t* world_create(struct allocator_t* alloc) {
    assert(alloc);
    world_t* w = BR_ALLOC(alloc, sizeof(world_t));
    memset(w, 0, sizeof(world_t));
    w->alloc = alloc;
    return w;
    
    /*
    for (size_t y = 0; y < WORLD_PLANE_SIZE; ++y) {
        for (size_t x = 0; x < WORLD_PLANE_SIZE; ++x) {
            if ((float)rand() / RAND_MAX > 0.5f) {
                s_ctx.current.world.locations[x][y].has_data  = true;
                s_ctx.current.world.locations[x][y].is_hidden = false;
                s_ctx.current.world.locations[x][y].terrain   = 1 + randi(TERRAIN_WATER_DEEP);
            } else {
                 s_ctx.current.world.locations[x][y].has_data  = true;
                 s_ctx.current.world.locations[x][y].is_hidden = true;
            }
        }
    }
    */
}

void world_free(struct world_t* w) {
    assert(w);
    BR_FREE(w->alloc, w);
}

void world_update(struct world_t* w, float dt) {
    assert(w);
    
    for (size_t i = 0; i < NUM_BLOCKS; ++i) {
        for (size_t j = 0; j < NUM_BLOCKS; ++j) {
            if (!w->has_data[i][j]) continue;
            
            w->age[i][j] += dt;
            if (w->age[i][j] > BLOCK_LIFE_SPAN_SECONDS) {
                w->age[i][j]      = 0.0f;
                w->has_data[i][j] = false;
            }
        }
    }
}

void world_update_data(struct world_t* w, struct api_map_t* data) {
    assert(w);
    assert(data);
    
    // TODO: @optimize Can calculate block spans and iterate per span.
    
    for (size_t i = 0; i < API_MAP_PART_SIZE; ++i) {
        for (size_t j = 0; j < API_MAP_PART_SIZE; ++j) {
            const int32_t tx = data->x + i;
            const int32_t ty = data->y + j;
            const block_index_t bi = to_block_index(tx, ty);
            
            w->has_data[bi.x][bi.y] = true;
            w->age[bi.x][bi.y]      = 0.0f;

            w->terrain[bi.x][bi.y].is_hidden = data->terrain[tx][ty].is_hidden;
            w->terrain[bi.x][bi.y].type      = data->terrain[tx][ty].terrain;
        }
    }
}

bool world_is_hidden(const struct world_t* w, int32_t x, int32_t y) {
    assert(w);
    assert(x >= 0 && x < WORLD_PLANE_SIZE);
    assert(y >= 0 && y < WORLD_PLANE_SIZE);
    
    const block_index_t bi = to_block_index(x, y);
    return !w->has_data[bi.x][bi.y] || w->terrain[bi.x][bi.y].is_hidden;
}

uint8_t world_terrain(const struct world_t* w, int32_t x, int32_t y) {
    assert(w);
    assert(x >= 0 && x < WORLD_PLANE_SIZE);
    assert(y >= 0 && y < WORLD_PLANE_SIZE);
    assert(!world_is_hidden(w, x, y));
    
    const block_index_t bi = to_block_index(x, y);
    return w->has_data[bi.x][bi.y] ? w->terrain[bi.x][bi.y].type : 0;
}
