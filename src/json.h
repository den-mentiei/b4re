#pragma once

// TODO: It is currently abandoned as not needed.
// Ideas is to have some kind of iterator into json_t with a bunch
// of accessor functions to extract values. And then build something
// like a tree-path walker to get nested values in high-level code.

struct allocator_t;

struct json_t;

struct json_t* json_parse(const char* data, struct allocator_t* alloc);
void json_free(struct json_t* json);
