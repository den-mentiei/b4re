#pragma once

#include <stddef.h>

struct allocator_t;

typedef struct pool_t pool_t;

pool_t* pool_create(size_t size, size_t initial_capacity, struct allocator_t* alloc);

void pool_destroy(pool_t* pool);

void* pool_alloc(pool_t* pool);

void pool_free(pool_t* pool, void* p);
