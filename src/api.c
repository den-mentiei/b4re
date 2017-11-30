#include "api.h"

#include <assert.h>
#include <stddef.h>

#include "log.h"
#include "json.h"
#include "world.h"

static void parse_resource(const struct json_t* json, json_iterator_t object, api_state_resource_t* res) {
	assert(json);
	assert(res);

#define NUMBER_PROPERTY(s) json_get_number(json, json_get_object_property(json, object, (s)))
	res->last_update     = NUMBER_PROPERTY("last_update");
	res->booster_time    = NUMBER_PROPERTY("booster_time");
	res->value           = NUMBER_PROPERTY("value");
	res->max             = NUMBER_PROPERTY("max");
	res->regen_rate      = NUMBER_PROPERTY("regen_rate");
	res->filled_segments = NUMBER_PROPERTY("filled_segments");
	res->segment_time    = NUMBER_PROPERTY("segment_time");
#undef NUMBER_PROPERTY
}

static void parse_player(const struct json_t* json, const json_iterator_t object, api_state_player_t* player) {
	assert(json);
	assert(player);

#define STRING_PROPERTY(s, f) json_get_string(json, json_get_object_property(json, object, (s)), (f), MAX_API_STRING_LENGTH);
	STRING_PROPERTY("username", &player->username[0]);
	STRING_PROPERTY("avatar",   &player->avatar[0]);
	STRING_PROPERTY("plane_id", &player->plane_id[0]);
#undef STRING_PROPERTY

#define NUMBER_PROPERTY(s) json_get_number(json, json_get_object_property(json, object, (s)))
	player->level = NUMBER_PROPERTY("level");
	player->exp   = NUMBER_PROPERTY("experience");
	player->money = NUMBER_PROPERTY("money");
	player->x     = NUMBER_PROPERTY("x");
	player->y     = NUMBER_PROPERTY("y");
#undef NUMBER_PROPERTY

	const json_iterator_t mind   = json_get_object_property(json, object, "mind");
	const json_iterator_t matter = json_get_object_property(json, object, "matter");

	parse_resource(json, mind,   &player->mind);
	parse_resource(json, matter, &player->matter);
}

bool api_parse_state(struct allocator_t* alloc, const char* data, api_state_t* state) {
	assert(alloc);
	assert(data);
	assert(state);

	struct json_t* json = json_parse(alloc, data);
	if (!json) return false;

	state->timestamp = json_get_number(json, json_get_object_property(json, 0, "timestamp"));

	const json_iterator_t player = json_get_object_property(json, 0, "player");
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
	/* const size_t data_length = strlen(data); */
	/* for (size_t i = 0; i < NUM_TERRAINS; ++i) { */
	/* 	const size_t lookup_length  = strlen(s_terrain_lookup[i].key); */
	/* 	const size_t compare_length = MIN(lookup_length, data_length); */
	/* 	if (lookup_length != data_length) continue; */
	/* 	if (strncmp(s_terrain_lookup[i].key, data, compare_length) == 0) { */
	/* 		return s_terrain_lookup[i].terrain; */
	/* 	} */
	/* } */
	return TERRAIN_DEFAULT;
}

bool api_parse_map(struct allocator_t* alloc, const char* data, api_map_t* map) {
	assert(alloc);
	assert(data);
	assert(map);

	/* json_t* json = json_parse(alloc, data); */
	/* if (!json) return false; */

	/* map->x = json_get_object_number(json, json->tokens, "x"); */
	/* map->y = json_get_object_number(json, json->tokens, "y"); */

	/* const json_iterator_t locations = json_get_value(json, json->tokens, "map"); */
	/* assert(locations); */
	
	/* for (size_t j = 0; j < locations->size; ++j) { */
	/* 	const json_iterator_t row = json_get_array_value(json, locations, j); */
	/* 	assert(row && row->type == JSMN_ARRAY); */
	/* 	for (size_t i = 0; i < row->size; ++i) { */
	/* 		const json_iterator_t loc = json_get_array_value(json, row, j); */
	/* 		assert(loc && loc->type == JSMN_OBJECT); */
			
	/* 		bool is_hidden = false; */
	/* 		try_get_object_bool(json, loc, "hidden", &is_hidden); */
	/* 		if (!is_hidden) { */
	/* 			char terrain_id[MAX_API_STRING_LENGTH]; */
	/* 			get_object_string(json, loc, "static_id", terrain_id, MAX_API_STRING_LENGTH); */

	/* 			map->terrain[i][j].is_hidden = false; */
	/* 			map->terrain[i][j].terrain   = parse_terrain(terrain_id); */
	/* 		} else { */
	/* 			map->terrain[i][j].is_hidden = true; */
	/* 		} */
	/* 	} */
	/* } */

	/* json_free(json); */
	return true;
}
