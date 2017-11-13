#pragma once

struct allocator_t;

struct json_t;

struct json_t* json_parse(struct allocator_t* alloc, const char* data);
void json_free(struct json_t* json);

typedef void json_value_t;
const json_value_t* json_get_value(const struct json_t* json, const char* data, const json_value_t* root, const char* key);
