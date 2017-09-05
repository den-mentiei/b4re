#pragma once

#include <stddef.h>

typedef struct allocator_t {
	void* (*realloc)(void* ctx, void* p, size_t new_size);
	void* payload;
} allocator_t;

#define BR_ALLOC(a, s)       (a)->realloc((a)->payload, NULL, (s));
#define BR_REALLOC(a, p, ns) (a)->realloc((a)->payload, p, (ns));
#define BR_FREE(a, p)        (a)->realloc((a)->payload, (p), 0);

const allocator_t* allocator_main();
