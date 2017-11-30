#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef size_t json_iterator_t;

struct allocator_t;

struct json_t* json_parse(struct allocator_t* alloc, const char* data);
void json_free(struct json_t* json);

json_iterator_t json_get_object_property(const struct json_t* json, json_iterator_t object, const char* key);
json_iterator_t json_get_array_value    (const struct json_t* json, json_iterator_t array, size_t i);
size_t          json_get_array_size     (const struct json_t* json, json_iterator_t array);

void     json_get_string(const struct json_t* json, json_iterator_t value, char* buffer, size_t size);
uint64_t json_get_number(const struct json_t* json, json_iterator_t value);
bool     json_get_bool  (const struct json_t* json, json_iterator_t value);
