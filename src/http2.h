#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// TODO: Think if we should return 0 if max in-flight requests are issued, or discard old ones.
// Client code can check if it's request is discarded by checking has(id), exposed in some way.

// 0 stands for a dummy always invalid id.
// TODO: Extern const for invalid one?
typedef uint32_t http2_work_id_t;

// Both key and value can point to a temporal storage, won't be used after post_form call.
typedef struct {
	const char* key;
	const char* value;
} http2_form_part_t;

void http2_init();

void http2_shutdown();

http2_work_id_t http2_get(const char* url, void* buffer, size_t size);

http2_work_id_t http2_post(const char* url, void* buffer, size_t size);

http2_work_id_t http2_post_form(const char* url, const http2_form_part_t* parts, size_t count, void* buffer, size_t size);

typedef enum {
	HTTP_STATUS_UNKNOWN = 0,
	HTTP_STATUS_IN_PROGRESS,
	HTTP_STATUS_FINISHED
} http2_status_t;

http2_status_t http2_status(http2_work_id_t id);
bool http2_response_code(http2_work_id_t id, uint8_t* code);
bool http2_response_size(http2_work_id_t id, size_t* size);
