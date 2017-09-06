#include "allocator.h"

#include <stdlib.h> // realloc

static void* std_realloc(void* ctx, void* p, size_t new_size) {
	return realloc(p, new_size);
}

static allocator_t main = {.realloc = std_realloc, .payload = NULL};

allocator_t* allocator_main() {
	return &main;
}
