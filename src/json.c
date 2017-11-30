#include "json.h"

#include <assert.h>
#include <stddef.h>
#include <string.h> // strlen, strncmp
#include <stdlib.h> // atoll

#include <jsmn.h>

#include "allocator.h"
#include "log.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct json_t {
	allocator_t* alloc;
	const char*  data;
	size_t       num_tokens;
	jsmntok_t*   tokens;
} json_t;

// HELPERS
// =======

static int dump(const char* data, const jsmntok_t* t, size_t count, int indent) {
	int i, j, k;

	if (count == 0) return 0;

	if (t->type == JSMN_PRIMITIVE) {
		log_info("%.*s", t->end - t->start, data+t->start);
		return 1;
	} else if (t->type == JSMN_STRING) {
		log_info("'%.*s'", t->end - t->start, data+t->start);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		log_info("");
		j = 0;
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent; k++) log_info("  ");
			j += dump(data, t+1+j, count-j, indent+1);
			log_info(": ");
			j += dump(data, t+1+j, count-j, indent+1);
			log_info("");
		}
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		log_info("");
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent-1; k++) log_info("  ");
			log_info(" - ");
			j += dump(data, t+1+j, count-j, indent+1);
			log_info("");
		}
		return j+1;
	}
	return 0;
}

static int skip(const char* data, const jsmntok_t* t, size_t count) {
	assert(data);
	assert(t);

	int j;

	if (count == 0) return 0;

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

// PARSERS
// =======

static inline bool is_number(const char* data, const jsmntok_t* t) {
	assert(data);
	assert(t);

	const char c = *(data + t->start);
	return t->type == JSMN_PRIMITIVE && (c == '-' || (c >= '0' && c <= '9')); 
}

static inline bool is_bool(const char* data, const jsmntok_t* t) {
	assert(data);
	assert(t);

	const char c = *(data + t->start);
	return t->type == JSMN_PRIMITIVE && (c == 't' || c == 'f'); 
}

static inline uint64_t parse_number_u64(const char* data, const jsmntok_t* t) {
	assert(is_number(data, t));
	return atoll(data + t->start);
}

static inline bool parse_bool(const char* data, const jsmntok_t* t) {
	assert(is_bool(data, t));
	return data[t->start] == 't';
}

// PUBLIC API
// ==========

json_t* json_parse(struct allocator_t* alloc, const char* data) {
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

void json_free(json_t* json) {
	assert(json);

	BR_FREE(json->alloc, json->tokens);
	BR_FREE(json->alloc, json);
}

json_iterator_t json_get_object_property(const json_t* json, json_iterator_t object, const char* key) {
	assert(json);
	assert(object < json->num_tokens);
	assert(key);

	const jsmntok_t* root = &json->tokens[object];
	assert(root->type == JSMN_OBJECT);

	const size_t key_length = strlen(key);
	const size_t left       = json->num_tokens - (root - json->tokens);

	size_t j = 0;
	for (size_t i = 0; i < root->size; ++i) {
		const jsmntok_t* kt = root + 1 + j;
		assert(kt->type == JSMN_STRING);

		const size_t value_length = kt->end - kt->start;
		if (value_length == key_length && strncmp(json->data + kt->start, key, key_length) == 0) {
			return object + 1 + j + 1;
		} else {
			j += 1 + skip(json->data, kt + 1, left - j);
		}
	}

	return object;
}

json_iterator_t json_get_array_value(const json_t* json, json_iterator_t array, size_t i) {
	assert(json);
	assert(array < json->num_tokens);

	const jsmntok_t* root = &json->tokens[array];
	assert(root->type == JSMN_ARRAY);
	assert(i < root->size);

	const size_t left = json->num_tokens - (root - json->tokens);

	size_t k = 0;
	for (size_t v = 0; v < i; ++v) {
		const jsmntok_t* kt = root + 1 + k;
		k += skip(json->data, kt, left - k);
	}

	return array + 1 + k;
}

size_t json_get_array_size(const json_t* json, const json_iterator_t array) {
	assert(json);
	assert(array < json->num_tokens);

	const jsmntok_t* root = &json->tokens[array];
	assert(root->type == JSMN_ARRAY);

	return root->size;
}

void json_get_string(const json_t* json, json_iterator_t value, char* buffer, size_t size) {
	assert(json);
	assert(value < json->num_tokens);
	assert(buffer);
	assert(size > 0);
	
	const jsmntok_t* string = &json->tokens[value];
	assert(string->type == JSMN_STRING);

	size_t value_length = string->end - string->start;
	size_t copy_length  = MIN(value_length, size - 1);
	memcpy(buffer, json->data + string->start, copy_length);
	buffer[copy_length] = 0;
}

uint64_t json_get_number(const json_t* json, json_iterator_t value) {
	return parse_number_u64(json->data, &json->tokens[value]);
}

bool json_get_bool(const json_t* json, json_iterator_t value) {
	return parse_bool(json->data, &json->tokens[value]);
}
