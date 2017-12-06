#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct allocator_t;
struct world_t;

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
		uint32_t level;
		uint32_t exp;
		int32_t  x, y;

		resource_t  mind;
		resource_t  matter;

		char    username[MAX_SESSION_STRING_LENGTH];
		uint8_t avatar;
	} player;
	
	struct world_t* world;
} session_t;

void session_init(struct allocator_t* alloc);
void session_update(float dt);
void session_shutdown();

const session_t* session_current();

// TODO: Remove it and let code directly use client_ stuff (rename it to commands or whatever).

void session_start(const char* username, const char* password);
void session_end();
void session_reveal(int32_t x, int32_t y);
typedef struct { int32_t tx; int32_t ty; } session_step_t;
void session_move(const session_step_t* steps, size_t count);
