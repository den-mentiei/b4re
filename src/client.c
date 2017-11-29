#include "client.h"

#include <stddef.h>

#include <stdio.h>

#include <assert.h>

#include "http.h"

#define RESPONSE_BUFFER_SIZE (8 * 1024)
#define RESPONSE_MESSAGE_BUFFER_SIZE (8 * 1024)
// Must be a power-of-two.
#define MAX_PAGES  16
#define ITEMS_MASK (MAX_PAGES - 1)

typedef struct {
	http_work_id_t request_id;

	uint8_t index;
	uint8_t next;

	uint8_t response_type;
	uint8_t response_buffer[RESPONSE_BUFFER_SIZE];
	uint8_t response_message[RESPONSE_MESSAGE_BUFFER_SIZE];
} page_t;

typedef struct {
	uint32_t  read;
	uint32_t  write;
	page_t*   items[MAX_PAGES];
} messages_t;

static struct {
	messages_t messages;

	page_t  pages[MAX_PAGES];
	uint8_t pages_free;

	page_t* pages_in_work[MAX_PAGES];
	uint8_t num_pages_in_work;
} s_ctx;

// PAGES MANAGEMENT
// ================

static void pages_init() {
	for (size_t i = 0; i < MAX_PAGES; ++i) {
		s_ctx.pages[i].index = i;
		s_ctx.pages[i].next  = i + 1;
	}
}

static page_t* pages_alloc() {
	assert(s_ctx.pages_free < MAX_PAGES);

	const size_t f = s_ctx.pages_free;
	s_ctx.pages_free = s_ctx.pages[f].next;
	return &s_ctx.pages[f];
}

static void pages_free(page_t* p) {
	assert(p);

	p->next          = s_ctx.pages_free;
	s_ctx.pages_free = p->index;
}

static void pages_put_in_work(page_t* p) {
	assert(p);
	assert(s_ctx.num_pages_in_work < MAX_PAGES);

	s_ctx.pages_in_work[s_ctx.num_pages_in_work++] = p;
}

static void pages_update() {
	page_t** pages = s_ctx.pages_in_work;

	size_t i = 0;
	size_t n = s_ctx.num_pages_in_work;

	while (i < n) {
		page_t* p = pages[i];

		// TODO: Handle unknown status by dropping it (can issue a message though).
		if (http_status(p->request_id) == HTTP_STATUS_FINISHED) {
			// TODO: Handle the response and issue a message.

			pages[i] = pages[n - 1];
			--n;
		} else {
			++i;
		}
	}

	s_ctx.num_pages_in_work = n;
}

// MESSAGES MANAGEMENT
// ===================

static size_t messages_size() {
	return s_ctx.messages.write - s_ctx.messages.read;
}

static bool messages_full() {
	return messages_size() == MAX_PAGES;
}

static bool messages_empty() {
	return s_ctx.messages.read == s_ctx.messages.write;
}

static void messages_push(page_t* p) {
	assert(!messages_full());
	assert(p);

	const uint32_t i = s_ctx.messages.write++;
	s_ctx.messages.items[i & ITEMS_MASK] = p;
}

static page_t* messages_peek() {
	assert(!messages_empty());
	const uint32_t i = s_ctx.messages.write;
	return s_ctx.messages.items[i & ITEMS_MASK];
}

static void messages_consume() {
	assert(!messages_empty());
	++s_ctx.messages.read;
}

// PUBLIC API
// ==========

void client_init() {
	pages_init();
}

void client_shutdown() {}

void client_update(float dt) {
	pages_update();
}

bool client_messages_peek(message_t** msg) {
	assert(msg);

	if (messages_empty()) return false;
	
	*msg = (message_t*)messages_peek()->response_message;
	return true;
}

void client_messages_consume() {
	page_t* p = messages_peek();
	messages_consume();
	pages_free(p);
}

void client_login(const char* username, const char* password) {
	assert(username);
	assert(password);

	// TODO: Copy it.
	http_form_part_t form[] = {
		{ "username", username },
		{ "password", password }
	};

	;
	page_t* p = pages_alloc();

	p->request_id = http_post_form(
		"http://ancientlighthouse.com:8080/api/login",
		form,
		sizeof(form) / sizeof(form[0]),
		p->response_buffer,
		RESPONSE_BUFFER_SIZE);

	pages_put_in_work(p);
}

void client_logout() {}

void client_state() {}

void client_move(uint8_t* coords, size_t count) {
	assert(coords);
	assert(count > 0);
}