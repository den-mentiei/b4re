#pragma once

#include <stddef.h>

typedef struct allocator_t allocator_t;

typedef struct pool_t pool_t;

pool_t* pool_create(size_t size, size_t initial_capacity, allocator_t* alloc);

void* pool_get(pool_t* pool);
void pool_release(pool_t* pool);

void pool_free(pool_t* pool);
