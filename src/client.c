#include "client.h"

#include <stddef.h>
#include <assert.h>
#include <stdio.h>  // snprintf

#include "log.h"
#include "http.h"

#define RESPONSE_BUFFER_SIZE (8 * 1024)
#define RESPONSE_MESSAGE_BUFFER_SIZE (8 * 1024)
// Must be a power-of-two.
#define MAX_PAGES  16
#define ITEMS_MASK (MAX_PAGES - 1)

typedef struct {
#ifdef DEBUG
	const char* tag;
#endif

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

// RESPONSE -> MESSAGE
// ===================

typedef void (*handler_t)();

void handle_noop() {}
void handle_login() {}
void handle_logout() {}
void handle_state() {}
void handle_map() {}
void handle_reveal() {}

static const struct { uint8_t t; handler_t h; }
HANDLERS[] = {
	{ MESSAGE_TYPE_NOOP,   handle_noop   },
	{ MESSAGE_TYPE_LOGIN,  handle_login  },
	{ MESSAGE_TYPE_LOGOUT, handle_logout },
	{ MESSAGE_TYPE_STATE,  handle_state  },
	{ MESSAGE_TYPE_MAP,    handle_map    },
	{ MESSAGE_TYPE_REVEAL, handle_reveal }
};
static const size_t NUM_HANDLERS = sizeof(HANDLERS) / sizeof(HANDLERS[0]);

static handler_t handlers_lookup(uint8_t type) {
	for (size_t i = 0; i < NUM_HANDLERS; ++i) {
		if (HANDLERS[i].t == type) return HANDLERS[i].h;
	}
	return HANDLERS[0].h;
}

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

static void pages_handle_response(page_t* p) {
	assert(p);

#ifdef DEBUG
	log_info("[client] Got a response for \"%s\"", p->tag);
#endif

	uint8_t code;
	if (http_response_code(p->request_id, &code)) {
		log_info("[client] Got a response code %u", code);
		size_t bytes;
		if (http_response_size(p->request_id, &bytes)) {
			log_info("[client] Got a response %zu bytes:", bytes);
			log_info("[client] %s", p->response_buffer);

			handler_t h = handlers_lookup(p->response_type);
			h();

			// TODO: Free a page?
		}
	}
}

static void pages_update() {
	page_t** pages_in_work = s_ctx.pages_in_work;

	size_t i = 0;
	size_t n = s_ctx.num_pages_in_work;

	while (i < n) {
		page_t* p = pages_in_work[i];

		// TODO: Handle unknown status by dropping it (can issue a message though).
		if (http_status(p->request_id) == HTTP_STATUS_FINISHED) {
			pages_handle_response(p);
			pages_in_work[i] = pages_in_work[n - 1];
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

	log_info("[client] Logging in with username=%s", username);

	// TODO: Copy it.
	http_form_part_t form[] = {
		{ "username", username },
		{ "password", password }
	};

	page_t* p = pages_alloc();

#ifdef DEBUG
	p->tag = "login";
#endif

	p->response_type = MESSAGE_TYPE_LOGIN;
	p->request_id    = http_post_form(
		"http://ancientlighthouse.com:8080/api/login",
		form,
		sizeof(form) / sizeof(form[0]),
		p->response_buffer,
		RESPONSE_BUFFER_SIZE);

	pages_put_in_work(p);
}

void client_logout() {
	log_info("[client] Logging out");
	/* http_post("http://ancientlighthouse.com:8080/api/logout", http_handler, NULL); */
}

void client_state() {
	log_info("[client] Fetching state");
	/* http_get("http://ancientlighthouse.com:8080/api/state", http_handler, STATE_TAG); */
}

void client_move(uint8_t* coords, size_t count) {
	assert(coords);
	assert(count > 0);
}

void client_map(uint32_t x, uint32_t y, uint8_t size) {
	assert(size > 0);
	log_info("[client] Fetching map");
/* 	http_get("http://ancientlighthouse.com:8080/api/map/homeland_3/0/0/12", http_handler, MAP_TAG); */
}

void client_reveal(uint32_t x, uint32_t y) {
	log_info("[client] Revealing %u, %u", x, y);
	char buf[128];
	snprintf(buf, sizeof(buf), "http://ancientlighthouse.com:8080/api/reveal/%u/%u", x, y);
	/* http_get(buf, http_handler, NULL); */
}
