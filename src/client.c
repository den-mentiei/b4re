#include "client.h"

#include <stddef.h>

#include <assert.h>

// TODO: Free-list of messages. Ring-buffer should contain a pointer to message.

// Must be a power-of-two.
#define MAX_MESSAGES  16
#define MESSAGES_MASK (MAX_MESSAGES - 1)

typedef struct {
	uint32_t  read;
	uint32_t  write;
	message_t items[MAX_MESSAGES];
} messages_t;

static struct {
	messages_t messages;
} s_ctx;

static size_t messages_size() {
	return s_ctx.messages.write - s_ctx.messages.read;
}

static bool messages_full() {
	return messages_size() == MAX_MESSAGES;
}

static bool messages_empty() {
	return s_ctx.messages.read == s_ctx.messages.write;
}

static void messages_push() {
	assert(!messages_full());

	uint32_t i = s_ctx.messages.write++;
	s_ctx.messages.items[i & MESSAGES_MASK].type = 1;
}

static void messages_pop() {
	assert(!messages_empty());

	uint32_t i = s_ctx.messages.read++;
	(void)s_ctx.messages.items[i & MESSAGES_MASK].type;
}

// PUBLIC API
// ==========

bool messages_get(message_t** msg) {
	assert(msg);

	if (messages_empty()) return false;
	
	// TODO:
	*msg = NULL;
	messages_pop();
	return true;
}
