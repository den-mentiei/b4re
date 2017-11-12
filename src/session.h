#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct resource_t {
	int32_t value;
	int32_t max;
} resource_t;

typedef struct session_t {
	struct {
		const char* username;
		const char* avatar;

		uint32_t    level;
		uint32_t    exp;
		uint32_t    x, y;

		resource_t  mind;
		resource_t  matter;
	} player;
} session_t;

struct allocator_t;

void session_init(struct allocator_t* alloc);
void session_update();
void session_shutdown();

const session_t* session_current();

void session_start(const char* username, const char* password);
void session_end();
