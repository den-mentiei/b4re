#include "session.h"

#include <assert.h>

#include "log.h"
#include "http.h"

static struct {
	session_t current;
} s_ctx;

/// API
static void login(const char* username, const char* password);
static void fetch_state();
/// API END

void session_init(struct allocator_t* alloc) {}

void session_shutdown() {}

const session_t* session_current() {
	return &s_ctx.current;
}

void session_start(const char* username, const char* password) {
	assert(!s_ctx.current.is_valid);
	assert(username);
	assert(password);

	// TODO: @network Issue a login request.

	login(username, password);

	s_ctx.current.player.username = username;
	s_ctx.current.is_valid = true;
}

// TODO: Move out?

/// API stuff

/* static int state; */

/* static void test_handler(const uint8_t* data, size_t size, void* payload) { */
/* 	log_info((const char*)data); */
/* 	log_info("\n"); */
/* 	state++; */
/* } */

static void http_handler(const uint8_t* data, size_t size, void* payload) {
	log_info((const char*)data);
	log_info("\n");
}

static void login(const char* username, const char* password) {
	log_info("[session] Login for u=%s\n", username);

	http_form_part_t form[] = {
		{ "username", username },
		{ "password", password }
	};

	http_post_form("http://ancientlighthouse.com:8080/api/login", form, sizeof(form) / sizeof(form[0]), http_handler, NULL);

	/* while (state != 1) {}; */

	/* http_get("http://ancientlighthouse.com:8080/api/state", test_handler, NULL); */

	/* while (state != 2) {}; */

	/* http_post("http://ancientlighthouse.com:8080/api/logout", test_handler, NULL); */
}

static void fetch_state() {
	http_get("http://ancientlighthouse.com:8080/api/state", http_handler, NULL);
}
