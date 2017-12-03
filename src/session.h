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

#define MAX_SESSION_STRING_LENGTH 64

typedef struct {
	struct {
		char username[MAX_SESSION_STRING_LENGTH];
		char avatar[MAX_SESSION_STRING_LENGTH];

		uint32_t level;
		uint32_t exp;
		int32_t  x, y;

		resource_t  mind;
		resource_t  matter;
	} player;

	world_t world;
} session_t;

void session_init();
void session_update(float dt);
void session_shutdown();

const session_t* session_current();

void session_start(const char* username, const char* password);
void session_end();

void session_reveal(uint32_t x, uint32_t y);
