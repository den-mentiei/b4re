#pragma once

#include <stdint.h>
#include <stddef.h>

struct allocator_t;

// TODO: status/headers/etc?
typedef void (*http_handler_t)(const uint8_t* data, size_t size, void* payload);

// Both key and value can point to a temporal storage, won't be used after post_form call.
typedef struct http_form_part_t {
	const char* key;
	const char* value;
} http_form_part_t;

void http_init(struct allocator_t* alloc);

void http_shutdown();

void http_get(const char* url, http_handler_t handler, void* payload);

void http_post(const char* url, http_handler_t handler, void* payload);

void http_post_form(const char* url, const http_form_part_t* parts, size_t count, http_handler_t handler, void* payload);
