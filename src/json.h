#pragma once

struct allocator_t;

struct json_t;

struct json_t* json_parse(const char* data, struct allocator_t* alloc);
void json_free(struct json_t* json);
