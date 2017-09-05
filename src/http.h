#pragma once

#include <stddef.h>

typedef struct allocator_t allocator_t;

void http_init(const allocator_t* alloc);

void http_get(const char* url);

// Both key and value can point to a temporal storage, won't be used after post_form call.
typedef struct http_form_part_t {
	const char* key;
	const char* value;
} http_form_part_t;

void http_post_form(const char* url, const http_form_part_t* parts, size_t count);

void http_shutdown();
