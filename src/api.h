#pragma once

#include <stdint.h>
#include <stdbool.h>

struct allocator_t;

#define MAX_API_STRING_LENGTH 64

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
} api_state_resource_t;

typedef enum {
	API_AVATAR_UNKNOWN = 0,
	API_AVATAR_MAN1,
	API_AVATAR_MAN2,
	API_AVATAR_MAN3,
	API_AVATAR_MAN4,
	API_AVATAR_MAN5,
	API_AVATAR_MAN6,
} api_avatar_t;

typedef struct {
	uint32_t level;
	uint32_t exp;
	uint32_t money;
	int32_t  x, y;

	api_state_resource_t mind;
	api_state_resource_t matter;

	char username[MAX_API_STRING_LENGTH];
	char plane_id[MAX_API_STRING_LENGTH];

	uint8_t avatar;

	// TODO: Movement, sigh. Skills. Skill keys. Under construction.
} api_state_player_t;

typedef struct {
	// Server time in seconds.
	uint64_t timestamp;
	api_state_player_t player;
} api_state_t;

#define API_MAP_PART_SIZE 12
typedef struct api_map_t {
	// TODO: @optimize Union?
	struct {
		uint8_t terrain;
		bool    is_hidden;
	} terrain[12][12];
	int32_t x, y;
} api_map_t;

bool api_parse_state(struct allocator_t* alloc, const char* data, api_state_t* state);
bool api_parse_map(struct allocator_t* alloc, const char* data, api_map_t* map);
