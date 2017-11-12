#include "json.h"

#include <assert.h>
#include <string.h> // strlen
#include <jsmn.h>

#include <stdio.h> // printf

#include "allocator.h"

typedef struct json_t {
	allocator_t* alloc;
	jsmntok_t*   tokens;
} json_t;

json_t* json_parse(const char* data, struct allocator_t* alloc) {
	assert(data);
	assert(alloc);

	jsmn_parser parser;
	jsmn_init(&parser);

	int num_tokens = jsmn_parse(&parser, data, strlen(data), NULL, 0);

	// TODO: @robustness Return NULL?
	assert(num_tokens >= 0);

	json_t* json = BR_ALLOC(alloc, sizeof(json_t));
	json->alloc  = alloc;
	json->tokens = BR_ALLOC(alloc, sizeof(jsmntok_t) * num_tokens);

	int parsed_tokens = jsmn_parse(&parser, data, strlen(data), json->tokens, num_tokens);

	// TODO: @robustness Return NULL?
	assert(parsed_tokens >= 0);
	assert(num_tokens == parsed_tokens);

	jsmntok_t* t = json->tokens;
	for (size_t i = 0; i < num_tokens; ++i) {
		printf("%zu %i %i->%i=%i\n", i, t[i].type, t[i].start, t[i].end, t[i].size);
	}

	return json;
}

void json_free(struct json_t* json) {
	assert(json);

	BR_FREE(json->alloc, json->tokens);
	BR_FREE(json->alloc, json);
}
