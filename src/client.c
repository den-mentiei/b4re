#include "client.h"

#include <stddef.h>
#include <assert.h>
#include <stdio.h>  // snprintf

#include "utils.h"
#include "log.h"
#include "http.h"
#include "allocator.h"
#include "api.h"

// TODO: Handle too-much-requests!

/* #define USE_LOCAL_SERVER */

#if defined(DEBUG) && defined(USE_LOCAL_SERVER)
	#define API_ENDPOINT(s) "http://localhost:8080/api/"s
#else
	#define API_ENDPOINT(s) "http://ancientlighthouse.com:8080/api/"s
#endif

#define RESPONSE_BUFFER_SIZE (4 * 8 * 1024)
#define RESPONSE_MESSAGE_BUFFER_SIZE (4 * 8 * 1024)
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

typedef void (*handler_t)(const void* response, void* out_msg);

void handle_noop(const void* response, void* out_msg) {
	assert(response);
	assert(out_msg);
}

void handle_login(const void* response, void* out_msg) {
	assert(response);
	assert(out_msg);
	// TODO: Nothing, I guess.
}

void handle_logout(const void* response, void* out_msg) {
	assert(response);
	assert(out_msg);
}

void handle_state(const void* response, void* out_msg) {
	assert(response);
	assert(out_msg);

	api_parse_state(allocator_main(), response, out_msg);
}

void handle_map(const void* response, void* out_msg) {
	assert(response);
	assert(out_msg);

	api_parse_map(allocator_main(), response, out_msg);
}

void handle_reveal(const void* response, void* out_msg) {
	assert(response);
	assert(out_msg);
}

static const struct { uint8_t t; handler_t h; }
HANDLERS[] = {
	{ MESSAGE_TYPE_NOOP,   handle_noop   },
	{ MESSAGE_TYPE_LOGIN,  handle_login  },
	{ MESSAGE_TYPE_LOGOUT, handle_logout },
	{ MESSAGE_TYPE_STATE,  handle_state  },
	{ MESSAGE_TYPE_MAP,    handle_map    },
	{ MESSAGE_TYPE_REVEAL, handle_reveal }
};

static handler_t handlers_lookup(uint8_t type) {
	for (size_t i = 0; i < ARRAY_SIZE(HANDLERS); ++i) {
		if (HANDLERS[i].t == type) return HANDLERS[i].h;
	}
	return HANDLERS[0].h;
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
	const uint32_t i = s_ctx.messages.read;
	return s_ctx.messages.items[i & ITEMS_MASK];
}

static void messages_consume() {
	assert(!messages_empty());
	++s_ctx.messages.read;
}

// PAGES MANAGEMENT
// ================

static void pages_init() {
	for (size_t i = 0; i < MAX_PAGES; ++i) {
		s_ctx.pages[i].index = i;
		s_ctx.pages[i].next  = i + 1;
	}
}

static page_t* pages_alloc(uint8_t type, const char* tag) {
	assert(s_ctx.pages_free < MAX_PAGES);

	const size_t f = s_ctx.pages_free;
	s_ctx.pages_free = s_ctx.pages[f].next;

	page_t* p = &s_ctx.pages[f];
	p->response_type = type;

#ifdef DEBUG
	p->tag = tag;
#else
	(void)tag;
#endif

	return p;
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
			/* log_info("[client] Got a response %zu bytes:", bytes); */
			/* log_info("[client] %s", p->response_buffer); */

			message_t* m = (message_t*)p->response_message;
			m->type = p->response_type;

			handler_t h = handlers_lookup(p->response_type);
			h(p->response_buffer, m->data);

			messages_push(p);
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

// API HELPERS
// ===========

static void api_get(const char* url, uint8_t type, const char* tag) {
	assert(url);

	page_t* p     = pages_alloc(type, tag);
	p->request_id = http_get(url, p->response_buffer, RESPONSE_BUFFER_SIZE);

	pages_put_in_work(p);
}

static void api_post_form(const char* url, const http_form_part_t* parts, size_t num_parts, uint8_t type, const char* tag) {
	assert(url);

	page_t* p     = pages_alloc(type, tag);
	p->request_id = http_post_form(url, parts, num_parts, p->response_buffer, RESPONSE_BUFFER_SIZE);

	pages_put_in_work(p);
}

static void api_post(const char* url, uint8_t type, const char* tag) {
	api_post_form(url, NULL, 0, type, tag);
}

static void api_post_json(const char* url, const char* payload, uint8_t type, const char* tag) {
	assert(url);
	assert(payload);

	page_t* p     = pages_alloc(type, tag);
	p->request_id = http_post_json(url, payload, p->response_buffer, RESPONSE_BUFFER_SIZE);

	pages_put_in_work(p);
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

	http_form_part_t form[] = {
		{ "username", username },
		{ "password", password }
	};

	api_post_form(API_ENDPOINT("login"), form, 2, MESSAGE_TYPE_LOGIN, "login");
}

void client_logout() {
	log_info("[client] Logging out");
	api_post(API_ENDPOINT("logout"), MESSAGE_TYPE_LOGOUT, "logout");
}

void client_state() {
	log_info("[client] Fetching state");
	api_get(API_ENDPOINT("state"), MESSAGE_TYPE_STATE, "state");
}

void client_move(const int32_t* coords, size_t count) {
	assert(coords);
	assert(count > 0);
	
	char buffer[512];
	size_t rest = 512;

	// TODO: Overflow validation.

	rest -= snprintf(buffer + (512 - rest), 512, "[ { \"x\": %d, \"y\": %d }\n", coords[0], coords[1]);
	for (size_t i = 1; i < count; ++i) {
		rest -= snprintf(buffer + (512 - rest), rest, ", { \"x\": %d, \"y\": %d }\n", coords[i * 2], coords[i * 2 + 1]);
	}
	snprintf(buffer + (512 - rest), rest, "]");
	
	api_post_json(API_ENDPOINT("move"), buffer, MESSAGE_TYPE_NOOP, "move");
}

void client_map(int32_t x, int32_t y, uint8_t size) {
	assert(size > 0);

	log_info("[client] Fetching map");

	char url[128];
	snprintf(url, sizeof(url), API_ENDPOINT("map/homeland_3/%d/%d/%u"), x, y, size);

	api_get(url, MESSAGE_TYPE_MAP, "map");
}

void client_reveal(int32_t x, int32_t y) {
	log_info("[client] Revealing %u, %u", x, y);

	char url[128];
	snprintf(url, sizeof(url), API_ENDPOINT("reveal/%u/%u"), x, y);

	api_post(url, MESSAGE_TYPE_REVEAL, "reveal");
}
