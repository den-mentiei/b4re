#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef size_t json_iterator_t;

struct allocator_t;

struct json_t* json_parse(struct allocator_t* alloc, const char* data);
void json_free(struct json_t* json);

json_iterator_t json_property    (const struct json_t* json, json_iterator_t object, const char* key);
bool            json_has_property(const struct json_t* json, json_iterator_t object, const char* key);
json_iterator_t json_array_value (const struct json_t* json, json_iterator_t array, size_t i);
size_t          json_array_size  (const struct json_t* json, json_iterator_t array);

void     json_string(const struct json_t* json, json_iterator_t value, char* buffer, size_t size);
uint64_t json_number(const struct json_t* json, json_iterator_t value);
bool     json_bool  (const struct json_t* json, json_iterator_t value);
