#include "session.h"

#include <assert.h>
#include <string.h> // memset, memcpy, strcpy

#include <tinycthread.h>

#include "allocator.h"
#include "log.h"
#include "http.h"

static struct {
	allocator_t alloc;

	session_t current;
	session_t synced;
	bool      is_active;

	char username[128];
	char avatar[128];

	mtx_t lock;
} s_ctx;

/// API
static void login(const char* username, const char* password);
static void logout();
static void fetch_state();
/// API END

void session_init(struct allocator_t* alloc) {
	if (mtx_init(&s_ctx.lock, mtx_plain) != thrd_success) log_fatal("[seession] failed to create mutex\n");
}

void session_update() {
	mtx_lock(&s_ctx.lock);
	memcpy(&s_ctx.current, &s_ctx.synced, sizeof(session_t));
	mtx_unlock(&s_ctx.lock);
}

void session_shutdown() {
	mtx_destroy(&s_ctx.lock);
}

const session_t* session_current() {
	return &s_ctx.current;
}

void session_start(const char* username, const char* password) {
	assert(!s_ctx.is_active);

	login(username, password);
}

void session_end() {
	assert(s_ctx.is_active);

	logout();

	// TODO: set it only if login was successfull.
	mtx_lock(&s_ctx.lock);
	memset(&s_ctx.synced,  0, sizeof(session_t));
	mtx_unlock(&s_ctx.lock);

	memset(&s_ctx.current, 0, sizeof(session_t));
}

// TODO: Move out?

/// API stuff

/* static int state; */

/* static void test_handler(const uint8_t* data, size_t size, void* payload) { */
/* 	log_info((const char*)data); */
/* 	log_info("\n"); */
/* 	state++; */
/* } */

static char* LOGIN_TAG = "login";

static void http_handler(const uint8_t* data, size_t size, void* payload) {
	if (payload == LOGIN_TAG) {
		mtx_lock(&s_ctx.lock);
		s_ctx.is_active = true;
		mtx_unlock(&s_ctx.lock);
	}

	log_info((const char*)data);
	log_info("\n");
}

static void login(const char* username, const char* password) {
	assert(username);
	assert(password);

	log_info("[session] Login with username=%s\n", username);

	http_form_part_t form[] = {
		{ "username", username },
		{ "password", password }
	};

	http_post_form("http://ancientlighthouse.com:8080/api/login", form, sizeof(form) / sizeof(form[0]), http_handler, LOGIN_TAG);

	/* while (state != 1) {}; */

	/* http_get("http://ancientlighthouse.com:8080/api/state", test_handler, NULL); */

	/* while (state != 2) {}; */
}

static void logout() {
	http_post("http://ancientlighthouse.com:8080/api/logout", http_handler, NULL);
}

static void fetch_state() {
	http_get("http://ancientlighthouse.com:8080/api/state", http_handler, NULL);
}
