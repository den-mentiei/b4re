#pragma once

#include <stdint.h>
#include <stdbool.h>

// TODO: @refactor Do *not* include!
#include "world.h"

typedef struct {
	// Server time in seconds.
	uint64_t last_update;
	// Booster time in seconds, 3600s per segment.
	uint64_t booster_time;

	uint8_t value;
	uint8_t max;

	// Regen rate, per segment: 0..9 seconds.
	uint8_t regen_rate;
	// Currently filled segments: 0..11 segments.
	uint8_t filled_segments;
	// Time in current segment: 0..<current rate> seconds.
	uint8_t segment_time;
} resource_t;

typedef struct {
	struct {
		const char* username;
		const char* avatar;

		uint32_t    level;
		uint32_t    exp;
		uint32_t    x, y;

		resource_t  mind;
		resource_t  matter;
	} player;

	world_t world;
} session_t;

struct allocator_t;

void session_init(struct allocator_t* alloc);
void session_update();
void session_shutdown();

const session_t* session_current();

void session_start(const char* username, const char* password);
void session_end();

void session_foo();
