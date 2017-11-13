#include "json.h"

#include <assert.h>
#include <string.h> // strlen, strncmp
#include <stdlib.h> // atoll
#include <stdbool.h>
#include <jsmn.h>

#include "allocator.h"
#include "log.h"

typedef struct json_t {
	allocator_t* alloc;
	size_t       num_tokens;
	jsmntok_t*   tokens;
} json_t;

void json_free(struct json_t* json) {
	assert(json);

	BR_FREE(json->alloc, json->tokens);
	BR_FREE(json->alloc, json);
}

json_t* json_parse(struct allocator_t* alloc, const char* data) {
	assert(data);
	assert(alloc);

	jsmn_parser parser;
	jsmn_init(&parser);

	int num_tokens = jsmn_parse(&parser, data, strlen(data), NULL, 0);
	if (num_tokens <= 0) return NULL;

	json_t* json     = BR_ALLOC(alloc, sizeof(json_t));
	json->alloc      = alloc;
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
				j += skip(data, t + 1 + j, count-j);
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

static inline bool is_number(const char* data, const jsmntok_t* t) {
	assert(t);
	assert(data);

	const char c = *(data + t->start);

	return t->type == JSMN_PRIMITIVE && (c == '-' || (c >= '0' && c <= '9')); 
}

static inline bool parse_uint64(const char* data, const jsmntok_t* t, uint64_t* out) {
	assert(out);

	if (!is_number(data, t)) {
		return false;
	}

	*out = atoll(data + t->start);
	return true;
}

const json_value_t* json_get_value(const json_t* json, const char* data, const json_value_t* root, const char* key) {
	assert(json);
	assert(key);

	if (json->num_tokens == 0) return NULL;

	const jsmntok_t* t = root ? root : json->tokens;
	const size_t left  = json->num_tokens - (t - json->tokens);

	if (t->type != JSMN_OBJECT) return NULL;

	size_t j = 0;
	for (size_t i = 0; i < t->size; ++i) {
		const jsmntok_t* kt = t + 1 + j;
		assert(kt->type == JSMN_STRING);
		if (strncmp(data + kt->start, key, kt->end - kt->start) == 0) {
			return kt + 1;
		} else {
			j += 1 + skip(data, kt + 1, left - j);
		}
	}

	return NULL;
}
