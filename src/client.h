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

bool messages_get(message_t** msg);
void messages_consume();

typedef enum {
	COMMAND_LOGIN,
	COMMAND_LOGOUT,
	COMMAND_STATE
} command_type_t;

typedef struct {
	uint8_t type;
	uint8_t data[];
} command_header_t;

typedef struct {
	char username[64];
	char password[64];
} command_login_t;

bool commands_add(const command_header_t* cmd);
