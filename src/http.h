#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// TODO: Think if we should return 0 if max in-flight requests are issued, or discard old ones.
// Client code can check if it's request is discarded by checking has(id), exposed in some way.

// 0 stands for a dummy always invalid id.
// TODO: Extern const for invalid one?
typedef uint32_t http_work_id_t;

// Both key and value can point to a temporal storage, won't be used after post_form call.
typedef struct {
	const char* key;
	const char* value;
} http_form_part_t;

void http_init();

void http_shutdown();

http_work_id_t http_get(const char* url, void* buffer, size_t size);

http_work_id_t http_post(const char* url, void* buffer, size_t size);

http_work_id_t http_post_form(const char* url, const http_form_part_t* parts, size_t num_parts, void* buffer, size_t size);

typedef enum {
	HTTP_STATUS_UNKNOWN = 0,
	HTTP_STATUS_IN_PROGRESS,
	HTTP_STATUS_FINISHED
} http_status_t;

http_status_t http_status(http_work_id_t id);
bool http_response_code(http_work_id_t id, uint8_t* code);
bool http_response_size(http_work_id_t id, size_t* size);
