#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
	MESSAGE_TYPE_STATE,
	MESSAGE_TYPE_MAP
} message_type_t;

typedef struct {
	uint8_t type;
	void*   data;
} message_t;

void client_init();
void client_shutdown();
void client_update(float dt);

void client_login(const char* username, const char* password);
void client_logout();
void client_state();
void client_move(uint8_t* coords, size_t count);

bool client_messages_peek(message_t** msg);
void client_messages_consume();

