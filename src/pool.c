#include "pool.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h> // memset

#include "allocator.h"

typedef struct {
	void* p;
	bool used;
} entry_t;

typedef struct pool_t {
	size_t size;
	allocator_t* alloc;
	entry_t* entries;
	size_t capacity;
	size_t free;
} pool_t;

// TODO: super not-optimal - allocate memory in pages and divide by size.

static void grow(pool_t* p, size_t new_capacity) {
	assert(p);
	assert(new_capacity > p->capacity);

	entry_t* e = p->entries = BR_REALLOC(p->alloc, p->entries, sizeof(entry_t) * new_capacity);

	allocator_t* a = p->alloc;
	for (size_t i = p->capacity; i < new_capacity; ++i) {
		e[i].p = BR_ALLOC(a, p->size);
		memset(e[i].p, 0, p->size);
	}

	p->capacity = new_capacity;
}

pool_t* pool_create(size_t size, size_t initial_capacity, allocator_t* alloc) {
	assert(size);
	assert(alloc);
	// TODO: enforce >0 initial capacity and hoist pool_t in the first allocation.

	pool_t* p = BR_ALLOC(alloc, sizeof(pool_t));
	p->size  = size;
	p->alloc = alloc;
	p->free  = 0;
	grow(p, initial_capacity);

	return p;
}

void* pool_get(pool_t* pool) {
	assert(pool);
}

void pool_release(pool_t* pool) {
	assert(pool);
}

void pool_free(pool_t* pool) {
	assert(pool);

	entry_t* e = pool->entries;
	for (size_t i = 0; i < pool->capacity; ++i) {
		BR_FREE(pool->alloc, e[i].p);
	}

	BR_FREE(pool->alloc, pool->entries);
	BR_FREE(pool->alloc, pool);
}
