#include "api.h"

#include <assert.h>
#include <string.h> // strlen, strncmp
#include <stdlib.h> // atoll
#include <jsmn.h>

#include "log.h"
#include "allocator.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// TODO: Error checking & reporting.

typedef struct json_t {
	allocator_t* alloc;
	const char*  data;
	size_t       num_tokens;
	jsmntok_t*   tokens;
} json_t;

static void json_free(struct json_t* json) {
	assert(json);

	BR_FREE(json->alloc, json->tokens);
	BR_FREE(json->alloc, json);
}

static json_t* json_parse(struct allocator_t* alloc, const char* data) {
	assert(data);
	assert(alloc);

	jsmn_parser parser;
	jsmn_init(&parser);

	int num_tokens = jsmn_parse(&parser, data, strlen(data), NULL, 0);
	if (num_tokens <= 0) return NULL;

	json_t* json     = BR_ALLOC(alloc, sizeof(json_t));
	json->alloc      = alloc;
	json->data       = data;
	json->num_tokens = num_tokens;
	json->tokens     = BR_ALLOC(alloc, sizeof(jsmntok_t) * num_tokens);

	jsmn_init(&parser);
	if (jsmn_parse(&parser, data, strlen(data), json->tokens, num_tokens) < 0) {
		json_free(json);
		return NULL;
	}

	return json;
}

static int dump(const char* data, const jsmntok_t* t, size_t count, int indent) {
	int i, j, k;
	if (count == 0) {
		return 0;
	}
	if (t->type == JSMN_PRIMITIVE) {
		log_info("%.*s", t->end - t->start, data+t->start);
		return 1;
	} else if (t->type == JSMN_STRING) {
		log_info("'%.*s'", t->end - t->start, data+t->start);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		log_info("\n");
		j = 0;
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent; k++) log_info("  ");
			j += dump(data, t+1+j, count-j, indent+1);
			log_info(": ");
			j += dump(data, t+1+j, count-j, indent+1);
			log_info("\n");
		}
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		log_info("\n");
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent-1; k++) log_info("  ");
			log_info("   - ");
			j += dump(data, t+1+j, count-j, indent+1);
			log_info("\n");
		}
		return j+1;
	}
	return 0;
}

static int skip(const char* data, const jsmntok_t* t, size_t count) {
	int j;

	if (count == 0) {
		return 0;
	}

	switch (t->type) {
		case JSMN_PRIMITIVE:
		case JSMN_STRING:
			return 1;

		case JSMN_OBJECT:
			j = 0;
			for (int i = 0; i < t->size; i++) {
				j += skip(data, t + 1 + j, count - j);
				j += skip(data, t+1+j, count-j);
			}
			return j + 1;

		case JSMN_ARRAY:
			j = 0;
			for (int i = 0; i < t->size; i++) {
				j += skip(data, t + 1 + j, count - j);
			}
			return j + 1;

		default:
			return 0;
	}
}

static const jsmntok_t* json_get_value(const json_t* json, const jsmntok_t* root, const char* key) {
	assert(json);
	assert(root);
	assert(key);

	if (json->num_tokens == 0)     return NULL;
	if (root->type != JSMN_OBJECT) return NULL;

	const size_t left = json->num_tokens - (root - json->tokens);

	size_t j = 0;
	for (size_t i = 0; i < root->size; ++i) {
		const jsmntok_t* kt = root + 1 + j;
		assert(kt->type == JSMN_STRING);
		// TODO: @robustness Calculate min between strlen(key) and key string.
		if (strncmp(json->data + kt->start, key, kt->end - kt->start) == 0) {
			return root + 1 + j + 1;
		} else {
			j += 1 + skip(json->data, kt + 1, left - j);
		}
	}

	return NULL;
}

static inline bool is_number(const char* data, const jsmntok_t* t) {
	assert(t);
	assert(data);

	const char c = *(data + t->start);
	return t->type == JSMN_PRIMITIVE && (c == '-' || (c >= '0' && c <= '9')); 
}

static inline bool try_parse_uint64(const char* data, const jsmntok_t* t, uint64_t* out) {
	assert(data);
	assert(t);
	assert(out);

	if (!t || !is_number(data, t)) return false;

	*out = atoll(data + t->start);
	return true;
}

static bool try_get_object_number(const json_t* json, const jsmntok_t* object, const char* key, uint64_t* out) {
	const jsmntok_t* value = json_get_value(json, object, key);
	return try_parse_uint64(json->data, value, out);
}

static uint64_t get_object_number(const json_t* json, const jsmntok_t* object, const char* key) {
	uint64_t number;
	if (!try_get_object_number(json, object, key, &number)) {
		log_error("[api] state: player: %s field is missing or not a number.\n", key);
		return 0;
	}
	return number;
}

static void get_object_string(const json_t* json, const jsmntok_t* object, const char* key, char* buf, size_t n) {
	const jsmntok_t* value = json_get_value(json, object, key);
	assert(value && value->type == JSMN_STRING);

	size_t value_length = value->end - value->start;
	size_t copy_length  = MIN(value_length, n - 1);
	memcpy(buf, json->data + value->start, copy_length);
	buf[copy_length] = 0;
}

static void parse_resource(const json_t* json, const jsmntok_t* object, api_state_resource_t* res) {
	assert(json);
	assert(object);
	assert(res);

	res->last_update     = get_object_number(json, object, "last_update");
	res->booster_time    = get_object_number(json, object, "booster_time");
	res->value           = get_object_number(json, object, "value");
	res->max             = get_object_number(json, object, "max");
	res->regen_rate      = get_object_number(json, object, "regen_rate");
	res->filled_segments = get_object_number(json, object, "filled_segments");
	res->segment_time    = get_object_number(json, object, "segment_time");
}

static void parse_player(const json_t* json, const jsmntok_t* object, api_state_player_t* player) {
	assert(json);
	assert(object && object->type == JSMN_OBJECT);
	assert(player);

	get_object_string(json, object, "username", &player->username[0], MAX_API_STRING_LENGTH);
	get_object_string(json, object, "avatar",   &player->avatar[0],   MAX_API_STRING_LENGTH);
	get_object_string(json, object, "plane_id", &player->plane_id[0], MAX_API_STRING_LENGTH);

	player->level = get_object_number(json, object, "level");
	player->exp   = get_object_number(json, object, "experience");
	player->money = get_object_number(json, object, "money");
	player->x     = get_object_number(json, object, "x");
	player->y     = get_object_number(json, object, "y");

	const jsmntok_t* mind   = json_get_value(json, object, "mind");
	const jsmntok_t* matter = json_get_value(json, object, "matter");
	assert(mind);
	assert(matter);

	parse_resource(json,   mind, &player->mind);
	parse_resource(json, matter, &player->matter);
}

bool api_parse_state(struct allocator_t* alloc, const char* data, api_state_t* state) {
	assert(alloc);
	assert(data);
	assert(state);

	json_t* json = json_parse(alloc, data);
	if (!json) return false;

	state->timestamp = get_object_number(json, json->tokens, "timestamp");

	const jsmntok_t* player = json_get_value(json, json->tokens, "player");
	assert(player);
	parse_player(json, player, &state->player);

	json_free(json);
	return true;
}