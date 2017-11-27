#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	MESSAGE_TYPE_STATE,
	MESSAGE_TYPE_MAP
} message_type_t;

typedef struct {
	uint8_t type;
	void*   data;
} message_t;

bool messages_get(message_t** msg);

