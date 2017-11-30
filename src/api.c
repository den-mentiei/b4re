#include "api.h"

#include <assert.h>
#include <stddef.h>
#include <string.h> // strncmp

#include "log.h"
#include "json.h"
#include "world.h"

#define BOOL_PROPERTY(j, v, s)      json_bool  ((j), json_property((j), (v), (s)))
#define NUMBER_PROPERTY(j, v, s)    json_number((j), json_property((j), (v), (s)))
#define STRING_PROPERTY(j, v, s, f) json_string((j), json_property((j), (v), (s)), (f), MAX_API_STRING_LENGTH);

static void parse_resource(const struct json_t* json, json_iterator_t object, api_state_resource_t* res) {
	assert(json);
	assert(res);

	res->last_update     = NUMBER_PROPERTY(json, object, "last_update");
	res->booster_time    = NUMBER_PROPERTY(json, object, "booster_time");
	res->value           = NUMBER_PROPERTY(json, object, "value");
	res->max             = NUMBER_PROPERTY(json, object, "max");
	res->regen_rate      = NUMBER_PROPERTY(json, object, "regen_rate");
	res->filled_segments = NUMBER_PROPERTY(json, object, "filled_segments");
	res->segment_time    = NUMBER_PROPERTY(json, object, "segment_time");
}

static void parse_player(const struct json_t* json, const json_iterator_t object, api_state_player_t* player) {
	assert(json);
	assert(player);

	STRING_PROPERTY(json, object, "username", &player->username[0]);
	STRING_PROPERTY(json, object, "avatar",   &player->avatar[0]);
	STRING_PROPERTY(json, object, "plane_id", &player->plane_id[0]);

	player->level = NUMBER_PROPERTY(json, object, "level");
	player->exp   = NUMBER_PROPERTY(json, object, "experience");
	player->money = NUMBER_PROPERTY(json, object, "money");
	player->x     = NUMBER_PROPERTY(json, object, "x");
	player->y     = NUMBER_PROPERTY(json, object, "y");

	const json_iterator_t mind   = json_property(json, object, "mind");
	const json_iterator_t matter = json_property(json, object, "matter");

	parse_resource(json, mind,   &player->mind);
	parse_resource(json, matter, &player->matter);
}

bool api_parse_state(struct allocator_t* alloc, const char* data, api_state_t* state) {
	assert(alloc);
	assert(data);
	assert(state);

	struct json_t* json = json_parse(alloc, data);
	if (!json) return false;

	state->timestamp = NUMBER_PROPERTY(json, 0, "timestamp");

	const json_iterator_t player = json_property(json, 0, "player");
	parse_player(json, player, &state->player);

	json_free(json);
	return true;
}

static const struct {
	const char* key;
	uint8_t terrain;
} s_terrain_lookup[] = {
	{ "rock_water",   TERRAIN_ROCK_WATER },
	{ "rock_solid",   TERRAIN_ROCK_SOLID },
	{ "rock",         TERRAIN_ROCK },
	{ "rock_sand",    TERRAIN_ROCK_SAND },
	{ "wild",         TERRAIN_WILD },
	{ "grass",        TERRAIN_GRASS },
	{ "earth",        TERRAIN_EARTH },
	{ "clay",         TERRAIN_CLAY },
	{ "sand",         TERRAIN_SAND },
	{ "water",        TERRAIN_WATER },
	{ "water_bottom", TERRAIN_WATER_BOTTOM },
	{ "water_deep",   TERRAIN_WATER_DEEP },
};
static const size_t NUM_TERRAINS = sizeof(s_terrain_lookup) / sizeof(s_terrain_lookup[0]);

static uint8_t parse_terrain(const char* data) {
	const size_t data_length = strlen(data);
	for (size_t i = 0; i < NUM_TERRAINS; ++i) {
		const size_t lookup_length  = strlen(s_terrain_lookup[i].key);
		const size_t compare_length = lookup_length < data_length ? lookup_length : data_length;
		if (lookup_length != data_length) continue;
		if (strncmp(s_terrain_lookup[i].key, data, compare_length) == 0) {
			return s_terrain_lookup[i].terrain;
		}
	}
	return TERRAIN_DEFAULT;
}

bool api_parse_map(struct allocator_t* alloc, const char* data, api_map_t* map) {
	assert(alloc);
	assert(data);
	assert(map);

	struct json_t* json = json_parse(alloc, data);
	if (!json) return false;

	map->x = NUMBER_PROPERTY(json, 0, "x");
	map->y = NUMBER_PROPERTY(json, 0, "y");

	const json_iterator_t locations = json_property(json, 0, "map");

	const size_t n = json_array_size(json, locations);
	
	for (size_t j = 0; j < n; ++j) {
		const json_iterator_t row      = json_array_value(json, locations, j);
		const size_t          row_size = json_array_size(json, row);
		for (size_t i = 0; i < row_size; ++i) {
			const json_iterator_t loc = json_array_value(json, row, j);
			
			bool is_hidden = false;
			if (json_has_property(json, loc, "hidden")) {
				is_hidden = BOOL_PROPERTY(json, loc, "hidden");
			}

			if (!is_hidden) {
				char terrain_id[MAX_API_STRING_LENGTH];
				STRING_PROPERTY(json, loc, "static_id", terrain_id);

				map->terrain[i][j].is_hidden = false;
				map->terrain[i][j].terrain   = parse_terrain(terrain_id);
			} else {
				map->terrain[i][j].is_hidden = true;
			}
		}
	}

	json_free(json);
	return true;
}
