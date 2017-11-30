#include "api.h"

#include <assert.h>
#include <stddef.h>

#include "log.h"
#include "json.h"
#include "world.h"

static void parse_resource(const struct json_t* json, const json_iterator_t object, api_state_resource_t* res) {
	assert(json);
	assert(object);
	assert(res);

	/* res->last_update     = json_get_object_number(json, object, "last_update"); */
	/* res->booster_time    = json_get_object_number(json, object, "booster_time"); */
	/* res->value           = json_get_object_number(json, object, "value"); */
	/* res->max             = json_get_object_number(json, object, "max"); */
	/* res->regen_rate      = json_get_object_number(json, object, "regen_rate"); */
	/* res->filled_segments = json_get_object_number(json, object, "filled_segments"); */
	/* res->segment_time    = json_get_object_number(json, object, "segment_time"); */
}

static void parse_player(const struct json_t* json, const json_iterator_t object, api_state_player_t* player) {
	assert(json);
	/* assert(object && object->type == JSMN_OBJECT); */
	assert(player);

	/* get_object_string(json, object, "username", &player->username[0], MAX_API_STRING_LENGTH); */
	/* get_object_string(json, object, "avatar",   &player->avatar[0],   MAX_API_STRING_LENGTH); */
	/* get_object_string(json, object, "plane_id", &player->plane_id[0], MAX_API_STRING_LENGTH); */

	/* player->level = json_get_object_number(json, object, "level"); */
	/* player->exp   = json_get_object_number(json, object, "experience"); */
	/* player->money = json_get_object_number(json, object, "money"); */
	/* player->x     = json_get_object_number(json, object, "x"); */
	/* player->y     = json_get_object_number(json, object, "y"); */

	/* const json_iterator_t mind   = json_get_value(json, object, "mind"); */
	/* const json_iterator_t matter = json_get_value(json, object, "matter"); */
	/* assert(mind); */
	/* assert(matter); */

	/* parse_resource(json,   mind, &player->mind); */
	/* parse_resource(json, matter, &player->matter); */
}

bool api_parse_state(struct allocator_t* alloc, const char* data, api_state_t* state) {
	assert(alloc);
	assert(data);
	assert(state);

	/* json_t* json = json_parse(alloc, data); */
	/* if (!json) return false; */

	/* state->timestamp = json_get_object_number(json, 0, "timestamp"); */

	/* const json_iterator_t player = json_get_value(json, 0, "player"); */
	/* assert(player); */
	/* parse_player(json, player, &state->player); */

	/* json_free(json); */
	/* return true; */
	return false;
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
