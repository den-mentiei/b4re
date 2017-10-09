#include "pool.h"

#include <stdbool.h>
#include <assert.h>

#include "allocator.h"

// TODO: Implement it!

struct pool_t {
	size_t size;
	allocator_t* alloc;
};

pool_t* pool_create(size_t size, size_t initial_capacity, allocator_t* alloc) {
	assert(size);
	assert(alloc);

	pool_t* p = BR_ALLOC(alloc, sizeof(pool_t));
	p->alloc  = alloc;
	p->size   = size;

	return p;
}

void pool_destroy(pool_t* pool) {
	assert(pool);

	BR_FREE(pool->alloc, pool);
}

void* pool_alloc(pool_t* pool) {
	assert(pool);
	// TODO:
	return BR_ALLOC(pool->alloc, pool->size);
}

void pool_free(pool_t* pool, void* p) {
	assert(pool);
	// TODO:
	BR_FREE(pool->alloc, p);
}
